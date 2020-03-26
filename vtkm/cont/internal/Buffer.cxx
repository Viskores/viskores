//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/internal/Assume.h>

#include <vtkm/cont/RuntimeDeviceInformation.h>

#include <vtkm/cont/internal/Buffer.h>
#include <vtkm/cont/internal/DeviceAdapterMemoryManager.h>

#include <condition_variable>
#include <map>

using LockType = std::unique_lock<std::mutex>;

namespace vtkm
{
namespace cont
{
namespace internal
{

class Buffer::InternalsStruct
{
public:
  using DeviceBufferMap =
    std::map<vtkm::cont::DeviceAdapterId, std::shared_ptr<vtkm::cont::internal::BufferInfo>>;
  using HostBufferPointer = std::shared_ptr<vtkm::cont::internal::BufferInfoHost>;

private:
  vtkm::cont::Token::ReferenceCount ReadCount = 0;
  vtkm::cont::Token::ReferenceCount WriteCount = 0;

  VTKM_CONT void CheckLock(const LockType& lock) const
  {
    VTKM_ASSERT((lock.mutex() == &this->Mutex) && (lock.owns_lock()));
  }

  // If this number disagrees with the size of the buffers, then they should be resized and
  // data preserved.
  vtkm::BufferSizeType NumberOfBytes = 0;

  DeviceBufferMap DeviceBuffers;
  HostBufferPointer HostBuffer;

public:
  std::mutex Mutex;
  std::condition_variable ConditionVariable;

  LockType GetLock() { return LockType(this->Mutex); }

  VTKM_CONT vtkm::cont::Token::ReferenceCount* GetReadCount(const LockType& lock)
  {
    this->CheckLock(lock);
    return &this->ReadCount;
  }
  VTKM_CONT vtkm::cont::Token::ReferenceCount* GetWriteCount(const LockType& lock)
  {
    this->CheckLock(lock);
    return &this->WriteCount;
  }

  VTKM_CONT DeviceBufferMap& GetDeviceBuffers(const LockType& lock)
  {
    this->CheckLock(lock);
    return this->DeviceBuffers;
  }

  VTKM_CONT HostBufferPointer& GetHostBuffer(const LockType& lock)
  {
    this->CheckLock(lock);
    return this->HostBuffer;
  }

  VTKM_CONT vtkm::BufferSizeType GetNumberOfBytes(const LockType& lock)
  {
    this->CheckLock(lock);
    return this->NumberOfBytes;
  }
  VTKM_CONT void SetNumberOfBytes(const LockType& lock, vtkm::BufferSizeType numberOfBytes)
  {
    this->CheckLock(lock);
    this->NumberOfBytes = numberOfBytes;
  }
};

namespace detail
{

struct BufferHelper
{
  static bool CanRead(Buffer::InternalsStruct* internals,
                      const LockType& lock,
                      const vtkm::cont::Token& token)
  {
    return ((*internals->GetWriteCount(lock) < 1) ||
            (token.IsAttached(internals->GetWriteCount(lock))));
  }

  static bool CanWrite(Buffer::InternalsStruct* internals,
                       const LockType& lock,
                       const vtkm::cont::Token& token)
  {
    return (
      ((*internals->GetWriteCount(lock) < 1) ||
       (token.IsAttached(internals->GetWriteCount(lock)))) &&
      ((*internals->GetReadCount(lock) < 1) ||
       ((*internals->GetReadCount(lock) == 1) && token.IsAttached(internals->GetReadCount(lock)))));
  }

  static void WaitToRead(Buffer::InternalsStruct* internals,
                         LockType& lock,
                         const vtkm::cont::Token& token)
  {
    // Note that if you deadlocked here, that means that you are trying to do a read operation on an
    // array where an object is writing to it. This could happen on the same thread. For example, if
    // you call `WritePortal()` then no other operation that can result in reading or writing
    // data in the array can happen while the resulting portal is still in scope.
    internals->ConditionVariable.wait(
      lock, [&lock, &token, internals] { return CanRead(internals, lock, token); });
  }

  static void WaitToWrite(Buffer::InternalsStruct* internals,
                          LockType& lock,
                          const vtkm::cont::Token& token)
  {
    // Note that if you deadlocked here, that means that you are trying to do a write operation on
    // an array where an object is reading or writing to it. This could happen on the same thread.
    // For example, if you call `WritePortal()` then no other operation that can result in reading
    // or writing data in the array can happen while the resulting portal is still in scope.
    internals->ConditionVariable.wait(
      lock, [&lock, &token, internals] { return CanWrite(internals, lock, token); });
  }

  static void AllocateOnHost(Buffer::InternalsStruct* internals,
                             std::unique_lock<std::mutex>& lock,
                             vtkm::cont::Token& token)
  {
    WaitToRead(internals, lock, token);
    Buffer::InternalsStruct::HostBufferPointer& hostBuffer = internals->GetHostBuffer(lock);
    vtkm::BufferSizeType targetSize = internals->GetNumberOfBytes(lock);
    if (hostBuffer.get() != nullptr)
    {
      // Buffer already exists on the host. Make sure it is the right size.
      if (hostBuffer->GetSize() != targetSize)
      {
        hostBuffer->Allocate(targetSize, vtkm::CopyFlag::On);
      }
      return;
    }

    // Buffer does not exist on host. See if we can find data on a device.
    for (auto&& deviceBuffer : internals->GetDeviceBuffers(lock))
    {
      if (deviceBuffer.second.get() == nullptr)
      {
        continue;
      }

      vtkm::cont::internal::DeviceAdapterMemoryManagerBase& memoryManager =
        vtkm::cont::RuntimeDeviceInformation().GetMemoryManager(deviceBuffer.first);

      if (deviceBuffer.second->GetSize() > targetSize)
      {
        // Device buffer too large. Resize.
        memoryManager.Reallocate(deviceBuffer.second, targetSize);
      }

      hostBuffer = memoryManager.CopyDeviceToHost(deviceBuffer.second);

      if (hostBuffer->GetSize() != targetSize)
      {
        hostBuffer->Allocate(targetSize, vtkm::CopyFlag::On);
      }

      return;
    }

    // Buffer not allocated on host or any device, so just allocate a buffer.
    hostBuffer.reset(new vtkm::cont::internal::BufferInfoHost);
    hostBuffer->Allocate(targetSize, vtkm::CopyFlag::Off);
  }

  static void AllocateOnDevice(Buffer::InternalsStruct* internals,
                               std::unique_lock<std::mutex>& lock,
                               vtkm::cont::Token& token,
                               vtkm::cont::DeviceAdapterId device)
  {
    WaitToRead(internals, lock, token);
    Buffer::InternalsStruct::DeviceBufferMap& deviceBuffers = internals->GetDeviceBuffers(lock);
    vtkm::BufferSizeType targetSize = internals->GetNumberOfBytes(lock);
    vtkm::cont::internal::DeviceAdapterMemoryManagerBase& memoryManager =
      vtkm::cont::RuntimeDeviceInformation().GetMemoryManager(device);

    if (deviceBuffers[device].get() != nullptr)
    {
      // Buffer already exists on the device. Make sure it is the right size.
      if (deviceBuffers[device]->GetSize() != targetSize)
      {
        memoryManager.Reallocate(deviceBuffers[device], targetSize);
      }
      VTKM_ASSERT(deviceBuffers[device]->GetSize() == targetSize);
      return;
    }

    // Buffer does not exist on device. Check to see if it is on another device but not a host.
    // We currently do not support device-to-device transfers, so the data has to go to the
    // host first.
    if (internals->GetHostBuffer(lock).get() == nullptr)
    {
      for (auto&& deviceBuffer : deviceBuffers)
      {
        if (deviceBuffer.second.get() != nullptr)
        {
          // Copy data to host.
          AllocateOnHost(internals, lock, token);
          break;
        }
      }
    }

    // If the buffer is now on the host, copy it to the device.
    Buffer::InternalsStruct::HostBufferPointer& hostBuffer = internals->GetHostBuffer(lock);
    if (hostBuffer.get() != nullptr)
    {
      if (hostBuffer->GetSize() > targetSize)
      {
        // Host buffer too large. Resize.
        hostBuffer->Allocate(targetSize, vtkm::CopyFlag::On);
      }

      deviceBuffers[device] = memoryManager.CopyHostToDevice(hostBuffer);

      if (deviceBuffers[device]->GetSize() != targetSize)
      {
        memoryManager.Reallocate(deviceBuffers[device], targetSize);
      }
      VTKM_ASSERT(deviceBuffers[device]->GetSize() == targetSize);

      return;
    }

    // Buffer not allocated anywhere, so just allocate a buffer.
    deviceBuffers[device] = memoryManager.Allocate(targetSize);
  }

  static void CopyOnHost(vtkm::cont::internal::Buffer::InternalsStruct* srcInternals,
                         LockType& srcLock,
                         vtkm::cont::internal::Buffer::InternalsStruct* destInternals,
                         LockType& destLock,
                         vtkm::cont::Token& token)
  {
    WaitToRead(srcInternals, srcLock, token);
    WaitToWrite(destInternals, destLock, token);

    // Any current buffers in destination can be (and should be) deleted.
    destInternals->GetDeviceBuffers(destLock).clear();

    // Do the copy
    Buffer::InternalsStruct::HostBufferPointer& destBuffer = destInternals->GetHostBuffer(destLock);
    Buffer::InternalsStruct::HostBufferPointer& srcBuffer = srcInternals->GetHostBuffer(srcLock);

    destBuffer.reset(new vtkm::cont::internal::BufferInfoHost);
    destBuffer->Allocate(srcBuffer->GetSize(), vtkm::CopyFlag::Off);

    std::copy(reinterpret_cast<const vtkm::UInt8*>(srcBuffer->GetPointer()),
              reinterpret_cast<const vtkm::UInt8*>(srcBuffer->GetPointer()) + srcBuffer->GetSize(),
              reinterpret_cast<vtkm::UInt8*>(destBuffer->GetPointer()));

    destInternals->SetNumberOfBytes(destLock, srcInternals->GetNumberOfBytes(srcLock));
  }

  static void CopyOnDevice(vtkm::cont::DeviceAdapterId device,
                           vtkm::cont::internal::Buffer::InternalsStruct* srcInternals,
                           LockType& srcLock,
                           vtkm::cont::internal::Buffer::InternalsStruct* destInternals,
                           LockType& destLock,
                           vtkm::cont::Token& token)
  {
    WaitToRead(srcInternals, srcLock, token);
    WaitToWrite(destInternals, destLock, token);

    // Any current buffers in destination can be (and should be) deleted.
    destInternals->GetHostBuffer(destLock).reset();
    destInternals->GetDeviceBuffers(destLock).clear();

    // Do the copy
    vtkm::cont::internal::DeviceAdapterMemoryManagerBase& memoryManager =
      vtkm::cont::RuntimeDeviceInformation().GetMemoryManager(device);

    destInternals->GetDeviceBuffers(destLock)[device] =
      memoryManager.CopyDeviceToDevice(srcInternals->GetDeviceBuffers(srcLock)[device]);

    destInternals->SetNumberOfBytes(destLock, srcInternals->GetNumberOfBytes(srcLock));
  }
};

} // namespace detail

Buffer::Buffer()
  : Internals(new InternalsStruct)
{
}

vtkm::BufferSizeType Buffer::GetNumberOfBytes() const
{
  LockType lock = this->Internals->GetLock();
  return this->Internals->GetNumberOfBytes(lock);
}

void Buffer::SetNumberOfBytes(vtkm::BufferSizeType numberOfBytes, vtkm::CopyFlag preserve)
{
  VTKM_ASSUME(numberOfBytes >= 0);
  LockType lock = this->Internals->GetLock();
  if (this->Internals->GetNumberOfBytes(lock) == numberOfBytes)
  {
    // Allocation has not changed. Just return.
    // Note, if you set the size to the old size and then try to get the buffer on a different
    // place, a copy might happen.
    return;
  }

  // We are altering the array, so make sure we can write to it.
  vtkm::cont::Token token;
  detail::BufferHelper::WaitToWrite(this->Internals.get(), lock, token);

  this->Internals->SetNumberOfBytes(lock, numberOfBytes);
  if ((preserve == vtkm::CopyFlag::Off) || (numberOfBytes == 0))
  {
    // No longer need these buffers. Just delete them.
    this->Internals->GetHostBuffer(lock).reset();
    this->Internals->GetDeviceBuffers(lock).clear();
  }
  else
  {
    // Do nothing (other than resetting numberOfBytes). Buffers will get resized when you get the
    // pointer.
  }
}

bool Buffer::IsAllocatedOnHost() const
{
  LockType lock = this->Internals->GetLock();
  return (this->Internals->GetHostBuffer(lock).get() != nullptr);
}

bool Buffer::IsAllocatedOnDevice(vtkm::cont::DeviceAdapterId device) const
{
  LockType lock = this->Internals->GetLock();
  return (this->Internals->GetDeviceBuffers(lock)[device].get() != nullptr);
}

const void* Buffer::ReadPointerHost(vtkm::cont::Token& token) const
{
  LockType lock = this->Internals->GetLock();
  detail::BufferHelper::WaitToRead(this->Internals.get(), lock, token);
  detail::BufferHelper::AllocateOnHost(this->Internals.get(), lock, token);
  return this->Internals->GetHostBuffer(lock)->GetPointer();
}

const void* Buffer::ReadPointerDevice(vtkm::cont::DeviceAdapterId device,
                                      vtkm::cont::Token& token) const
{
  LockType lock = this->Internals->GetLock();
  detail::BufferHelper::WaitToRead(this->Internals.get(), lock, token);
  detail::BufferHelper::AllocateOnDevice(this->Internals.get(), lock, token, device);
  return this->Internals->GetDeviceBuffers(lock)[device]->GetPointer();
}

void* Buffer::WritePointerHost(vtkm::cont::Token& token) const
{
  LockType lock = this->Internals->GetLock();
  detail::BufferHelper::WaitToWrite(this->Internals.get(), lock, token);
  detail::BufferHelper::AllocateOnHost(this->Internals.get(), lock, token);

  // Array is being written on host. All other buffers invalidated, so delete them.
  this->Internals->GetDeviceBuffers(lock).clear();

  return this->Internals->GetHostBuffer(lock)->GetPointer();
}

void* Buffer::WritePointerDevice(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token& token) const
{
  LockType lock = this->Internals->GetLock();
  detail::BufferHelper::WaitToWrite(this->Internals.get(), lock, token);
  detail::BufferHelper::AllocateOnDevice(this->Internals.get(), lock, token, device);

  // Array is being written on this device. All other buffers invalided, so delete them.
  this->Internals->GetHostBuffer(lock).reset();
  InternalsStruct::DeviceBufferMap& deviceBuffers = this->Internals->GetDeviceBuffers(lock);
  auto iter = deviceBuffers.find(device);
  deviceBuffers.erase(deviceBuffers.begin(), iter);
  iter = deviceBuffers.find(device);
  std::advance(iter, 1);
  deviceBuffers.erase(iter, deviceBuffers.end());

  return this->Internals->GetDeviceBuffers(lock)[device]->GetPointer();
}

void Buffer::DeepCopy(vtkm::cont::internal::Buffer& dest) const
{
  LockType srcLock = this->Internals->GetLock();
  LockType destLock = dest.Internals->GetLock();
  vtkm::cont::Token token;

  detail::BufferHelper::WaitToRead(this->Internals.get(), srcLock, token);

  if (!this->Internals->GetDeviceBuffers(srcLock).empty())
  {
    // If we are on a device, copy there.
    detail::BufferHelper::CopyOnDevice(this->Internals->GetDeviceBuffers(srcLock).begin()->first,
                                       this->Internals.get(),
                                       srcLock,
                                       dest.Internals.get(),
                                       destLock,
                                       token);
  }
  else if (this->Internals->GetHostBuffer(srcLock) != nullptr)
  {
    // If we are on a host, copy there
    detail::BufferHelper::CopyOnHost(
      this->Internals.get(), srcLock, dest.Internals.get(), destLock, token);
  }
  else
  {
    // Nothing actually allocated. Just unallocate everything on destination.
    dest.Internals->GetHostBuffer(destLock).reset();
    dest.Internals->GetDeviceBuffers(destLock).clear();
    dest.Internals->SetNumberOfBytes(destLock, this->Internals->GetNumberOfBytes(srcLock));
  }
}

void Buffer::DeepCopy(vtkm::cont::internal::Buffer& dest, vtkm::cont::DeviceAdapterId device) const
{
  LockType srcLock = this->Internals->GetLock();
  LockType destLock = dest.Internals->GetLock();
  vtkm::cont::Token token;
  detail::BufferHelper::CopyOnDevice(
    device, this->Internals.get(), srcLock, dest.Internals.get(), destLock, token);
}

void Buffer::Reset(const std::shared_ptr<vtkm::UInt8> buffer, vtkm::BufferSizeType numberOfBytes)
{
  LockType lock = this->Internals->GetLock();
  vtkm::cont::Token token;
  detail::BufferHelper::WaitToWrite(this->Internals.get(), lock, token);

  this->Internals->GetDeviceBuffers(lock).clear();

  this->Internals->GetHostBuffer(lock).reset(
    new vtkm::cont::internal::BufferInfoHost(buffer, numberOfBytes));
  this->Internals->SetNumberOfBytes(lock, numberOfBytes);
}

void Buffer::Reset(const std::shared_ptr<vtkm::UInt8> buffer,
                   vtkm::BufferSizeType numberOfBytes,
                   vtkm::cont::DeviceAdapterId device)
{
  LockType lock = this->Internals->GetLock();
  vtkm::cont::Token token;
  detail::BufferHelper::WaitToWrite(this->Internals.get(), lock, token);

  this->Internals->GetDeviceBuffers(lock).clear();
  this->Internals->GetHostBuffer(lock).reset();

  this->Internals->GetDeviceBuffers(lock)[device] =
    vtkm::cont::RuntimeDeviceInformation().GetMemoryManager(device).ManageArray(buffer,
                                                                                numberOfBytes);
  this->Internals->SetNumberOfBytes(lock, numberOfBytes);
}
}
}
} // namespace vtkm::cont::internal

namespace mangled_diy_namespace
{

void Serialization<vtkm::cont::internal::Buffer>::save(BinaryBuffer& bb,
                                                       const vtkm::cont::internal::Buffer& obj)
{
  vtkm::BufferSizeType size = obj.GetNumberOfBytes();
  vtkmdiy::save(bb, size);

  vtkm::cont::Token token;
  const vtkm::UInt8* data = reinterpret_cast<const vtkm::UInt8*>(obj.ReadPointerHost(token));
  vtkmdiy::save(bb, data, static_cast<std::size_t>(size));
}

void Serialization<vtkm::cont::internal::Buffer>::load(BinaryBuffer& bb,
                                                       vtkm::cont::internal::Buffer& obj)
{
  vtkm::BufferSizeType size;
  vtkmdiy::load(bb, size);

  obj.SetNumberOfBytes(size, vtkm::CopyFlag::Off);
  vtkm::cont::Token token;
  vtkm::UInt8* data = reinterpret_cast<vtkm::UInt8*>(obj.WritePointerHost(token));
  vtkmdiy::load(bb, data, static_cast<std::size_t>(size));
}

} // namespace diy
