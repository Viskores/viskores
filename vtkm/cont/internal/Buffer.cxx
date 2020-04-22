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

#include <vtkm/cont/ErrorBadDevice.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>

#include <vtkm/cont/internal/Buffer.h>
#include <vtkm/cont/internal/DeviceAdapterMemoryManager.h>

#include <condition_variable>
#include <cstring>
#include <deque>
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
  using DeviceBufferMap = std::map<vtkm::cont::DeviceAdapterId, vtkm::cont::internal::BufferInfo>;

private:
  vtkm::cont::Token::ReferenceCount ReadCount = 0;
  vtkm::cont::Token::ReferenceCount WriteCount = 0;

  std::deque<vtkm::cont::Token::Reference> Queue;

  VTKM_CONT void CheckLock(const LockType& lock) const
  {
    VTKM_ASSERT((lock.mutex() == &this->Mutex) && (lock.owns_lock()));
  }

  // If this number disagrees with the size of the buffers, then they should be resized and
  // data preserved.
  vtkm::BufferSizeType NumberOfBytes = 0;

  DeviceBufferMap DeviceBuffers;
  vtkm::cont::internal::BufferInfo HostBuffer;

public:
  std::mutex Mutex;
  std::condition_variable ConditionVariable;

  LockType GetLock() { return LockType(this->Mutex); }

  VTKM_CONT std::deque<vtkm::cont::Token::Reference>& GetQueue(const LockType& lock)
  {
    this->CheckLock(lock);
    return this->Queue;
  }

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

  VTKM_CONT vtkm::cont::internal::BufferInfo& GetHostBuffer(const LockType& lock)
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

struct VTKM_NEVER_EXPORT BufferHelper
{
  enum struct AccessMode
  {
    READ,
    WRITE
  };

  static void Enqueue(const std::shared_ptr<Buffer::InternalsStruct>& internals,
                      const LockType& lock,
                      const vtkm::cont::Token& token)
  {
    if (token.IsAttached(internals->GetWriteCount(lock)) ||
        token.IsAttached(internals->GetReadCount(lock)))
    {
      // Do not need to enqueue if we are already attached.
      return;
    }

    auto& queue = internals->GetQueue(lock);
    if (std::find(queue.begin(), queue.end(), token.GetReference()) != queue.end())
    {
      // This token is already in the queue.
      return;
    }

    queue.push_back(token.GetReference());
  }

  static bool CanRead(const std::shared_ptr<Buffer::InternalsStruct>& internals,
                      const LockType& lock,
                      const vtkm::cont::Token& token)
  {
    // If the token is already attached to this array, then we allow reading.
    if (token.IsAttached(internals->GetWriteCount(lock)) ||
        token.IsAttached(internals->GetReadCount(lock)))
    {
      return true;
    }

    // If there is anyone else waiting at the top of the queue, we cannot access this array.
    auto& queue = internals->GetQueue(lock);
    if (!queue.empty() && (queue.front() != token))
    {
      return false;
    }

    // No one else is waiting, so we can read the buffer as long as no one else is writing.
    return (*internals->GetWriteCount(lock) < 1);
  }

  static bool CanWrite(const std::shared_ptr<Buffer::InternalsStruct>& internals,
                       const LockType& lock,
                       const vtkm::cont::Token& token)
  {
    // If the token is already attached to this array, then we allow writing.
    if (token.IsAttached(internals->GetWriteCount(lock)) ||
        token.IsAttached(internals->GetReadCount(lock)))
    {
      return true;
    }

    // If there is anyone else waiting at the top of the queue, we cannot access this array.
    auto& queue = internals->GetQueue(lock);
    if (!queue.empty() && (queue.front() != token))
    {
      return false;
    }

    // No one else is waiting, so we can write the buffer as long as no one else is reading
    // or writing.
    return ((*internals->GetWriteCount(lock) < 1) && (*internals->GetReadCount(lock) < 1));
  }

  static void WaitToRead(const std::shared_ptr<Buffer::InternalsStruct>& internals,
                         LockType& lock,
                         vtkm::cont::Token& token)
  {
    Enqueue(internals, lock, token);

    // Note that if you deadlocked here, that means that you are trying to do a read operation on an
    // array where an object is writing to it. This could happen on the same thread. For example, if
    // you call `WritePortal()` then no other operation that can result in reading or writing
    // data in the array can happen while the resulting portal is still in scope.
    internals->ConditionVariable.wait(
      lock, [&lock, &token, internals] { return CanRead(internals, lock, token); });

    token.Attach(internals, internals->GetReadCount(lock), lock, &internals->ConditionVariable);

    // We successfully attached the token. Pop it off the queue.
    auto& queue = internals->GetQueue(lock);
    if (!queue.empty() && queue.front() == token)
    {
      queue.pop_front();
    }
  }

  static void WaitToWrite(const std::shared_ptr<Buffer::InternalsStruct>& internals,
                          LockType& lock,
                          vtkm::cont::Token& token)
  {
    Enqueue(internals, lock, token);

    // Note that if you deadlocked here, that means that you are trying to do a write operation on
    // an array where an object is reading or writing to it. This could happen on the same thread.
    // For example, if you call `WritePortal()` then no other operation that can result in reading
    // or writing data in the array can happen while the resulting portal is still in scope.
    internals->ConditionVariable.wait(
      lock, [&lock, &token, internals] { return CanWrite(internals, lock, token); });

    token.Attach(internals, internals->GetWriteCount(lock), lock, &internals->ConditionVariable);

    // We successfully attached the token. Pop it off the queue.
    auto& queue = internals->GetQueue(lock);
    if (!queue.empty() && queue.front() == token)
    {
      queue.pop_front();
    }
  }

  static void Wait(const std::shared_ptr<Buffer::InternalsStruct>& internals,
                   LockType& lock,
                   vtkm::cont::Token& token,
                   AccessMode accessMode)
  {
    switch (accessMode)
    {
      case AccessMode::READ:
        WaitToRead(internals, lock, token);
        break;
      case AccessMode::WRITE:
        WaitToWrite(internals, lock, token);
        break;
    }
  }

  static void AllocateOnHost(const std::shared_ptr<Buffer::InternalsStruct>& internals,
                             std::unique_lock<std::mutex>& lock,
                             vtkm::cont::Token& token,
                             AccessMode accessMode)
  {
    Wait(internals, lock, token, accessMode);
    vtkm::cont::internal::BufferInfo& hostBuffer = internals->GetHostBuffer(lock);
    vtkm::BufferSizeType targetSize = internals->GetNumberOfBytes(lock);
    if (hostBuffer.GetPointer() != nullptr)
    {
      // Buffer already exists on the host. Make sure it is the right size.
      if (hostBuffer.GetSize() != targetSize)
      {
        hostBuffer.Reallocate(targetSize);
      }
      return;
    }

    // Buffer does not exist on host. See if we can find data on a device.
    for (auto&& deviceBuffer : internals->GetDeviceBuffers(lock))
    {
      if (deviceBuffer.second.GetPointer() == nullptr)
      {
        continue;
      }

      vtkm::cont::internal::DeviceAdapterMemoryManagerBase& memoryManager =
        vtkm::cont::RuntimeDeviceInformation().GetMemoryManager(deviceBuffer.first);

      if (deviceBuffer.second.GetSize() > targetSize)
      {
        // Device buffer too large. Resize.
        memoryManager.Reallocate(deviceBuffer.second, targetSize);
      }

      hostBuffer = memoryManager.CopyDeviceToHost(deviceBuffer.second);

      if (hostBuffer.GetSize() != targetSize)
      {
        hostBuffer.Reallocate(targetSize);
      }

      return;
    }

    // Buffer not allocated on host or any device, so just allocate a buffer.
    hostBuffer = vtkm::cont::internal::AllocateOnHost(targetSize);
  }

  static void AllocateOnDevice(const std::shared_ptr<Buffer::InternalsStruct>& internals,
                               std::unique_lock<std::mutex>& lock,
                               vtkm::cont::Token& token,
                               vtkm::cont::DeviceAdapterId device,
                               AccessMode accessMode)
  {
    Wait(internals, lock, token, accessMode);
    Buffer::InternalsStruct::DeviceBufferMap& deviceBuffers = internals->GetDeviceBuffers(lock);
    vtkm::BufferSizeType targetSize = internals->GetNumberOfBytes(lock);
    vtkm::cont::internal::DeviceAdapterMemoryManagerBase& memoryManager =
      vtkm::cont::RuntimeDeviceInformation().GetMemoryManager(device);

    if (deviceBuffers[device].GetPointer() != nullptr)
    {
      // Buffer already exists on the device. Make sure it is the right size.
      if (deviceBuffers[device].GetSize() != targetSize)
      {
        deviceBuffers[device].Reallocate(targetSize);
      }
      VTKM_ASSERT(deviceBuffers[device].GetSize() == targetSize);
      return;
    }

    // Buffer does not exist on device. Check to see if it is on another device but not the host.
    // We currently do not support device-to-device transfers, so the data has to go to the
    // host first.
    if (internals->GetHostBuffer(lock).GetPointer() == nullptr)
    {
      for (auto&& deviceBuffer : deviceBuffers)
      {
        if (deviceBuffer.second.GetPointer() != nullptr)
        {
          // Copy data to host.
          AllocateOnHost(internals, lock, token, accessMode);
          break;
        }
      }
    }

    // If the buffer is now on the host, copy it to the device.
    vtkm::cont::internal::BufferInfo& hostBuffer = internals->GetHostBuffer(lock);
    if (hostBuffer.GetPointer() != nullptr)
    {
      if (hostBuffer.GetSize() > targetSize)
      {
        // Host buffer too large. Resize.
        hostBuffer.Reallocate(targetSize);
      }

      deviceBuffers[device] = memoryManager.CopyHostToDevice(hostBuffer);

      if (deviceBuffers[device].GetSize() != targetSize)
      {
        deviceBuffers[device].Reallocate(targetSize);
      }
      VTKM_ASSERT(deviceBuffers[device].GetSize() == targetSize);

      return;
    }

    // Buffer not allocated anywhere, so just allocate a buffer.
    deviceBuffers[device] = memoryManager.Allocate(targetSize);
  }

  static void CopyOnHost(
    const std::shared_ptr<vtkm::cont::internal::Buffer::InternalsStruct>& srcInternals,
    LockType& srcLock,
    const std::shared_ptr<vtkm::cont::internal::Buffer::InternalsStruct>& destInternals,
    LockType& destLock,
    vtkm::cont::Token& token)
  {
    WaitToRead(srcInternals, srcLock, token);
    WaitToWrite(destInternals, destLock, token);

    // Any current buffers in destination can be (and should be) deleted.
    destInternals->GetDeviceBuffers(destLock).clear();

    // Do the copy
    vtkm::cont::internal::BufferInfo& destBuffer = destInternals->GetHostBuffer(destLock);
    vtkm::cont::internal::BufferInfo& srcBuffer = srcInternals->GetHostBuffer(srcLock);

    destBuffer = vtkm::cont::internal::AllocateOnHost(srcBuffer.GetSize());

    std::memcpy(destBuffer.GetPointer(),
                srcBuffer.GetPointer(),
                static_cast<std::size_t>(srcBuffer.GetSize()));

    destInternals->SetNumberOfBytes(destLock, srcInternals->GetNumberOfBytes(srcLock));
  }

  static void CopyOnDevice(
    vtkm::cont::DeviceAdapterId device,
    const std::shared_ptr<vtkm::cont::internal::Buffer::InternalsStruct>& srcInternals,
    LockType& srcLock,
    const std::shared_ptr<vtkm::cont::internal::Buffer::InternalsStruct>& destInternals,
    LockType& destLock,
    vtkm::cont::Token& token)
  {
    WaitToRead(srcInternals, srcLock, token);
    WaitToWrite(destInternals, destLock, token);

    // Any current buffers in destination can be (and should be) deleted.
    destInternals->GetHostBuffer(destLock) = vtkm::cont::internal::BufferInfo{};
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

// Defined to prevent issues with CUDA
Buffer::Buffer(const Buffer& src)
  : Internals(src.Internals)
{
}

// Defined to prevent issues with CUDA
Buffer::Buffer(Buffer&& src)
  : Internals(std::move(src.Internals))
{
}

// Defined to prevent issues with CUDA
Buffer::~Buffer()
{
}

// Defined to prevent issues with CUDA
Buffer& Buffer::operator=(const Buffer& src)
{
  this->Internals = src.Internals;
  return *this;
}

// Defined to prevent issues with CUDA
Buffer& Buffer::operator=(Buffer&& src)
{
  this->Internals = std::move(src.Internals);
  return *this;
}

vtkm::BufferSizeType Buffer::GetNumberOfBytes() const
{
  LockType lock = this->Internals->GetLock();
  return this->Internals->GetNumberOfBytes(lock);
}

void Buffer::SetNumberOfBytes(vtkm::BufferSizeType numberOfBytes, vtkm::CopyFlag preserve)
{
  VTKM_ASSUME(numberOfBytes >= 0);
  // A Token should not be declared within the scope of a lock. When the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->Internals->GetLock();
    if (this->Internals->GetNumberOfBytes(lock) == numberOfBytes)
    {
      // Allocation has not changed. Just return.
      // Note, if you set the size to the old size and then try to get the buffer on a different
      // place, a copy might happen.
      return;
    }

    // We are altering the array, so make sure we can write to it.
    detail::BufferHelper::WaitToWrite(this->Internals, lock, token);

    this->Internals->SetNumberOfBytes(lock, numberOfBytes);
    if ((preserve == vtkm::CopyFlag::Off) || (numberOfBytes == 0))
    {
      // No longer need these buffers. Just delete them.
      this->Internals->GetHostBuffer(lock) = vtkm::cont::internal::BufferInfo{};
      this->Internals->GetDeviceBuffers(lock).clear();
    }
    else
    {
      // Do nothing (other than resetting numberOfBytes). Buffers will get resized when you get the
      // pointer.
    }
  }
}

bool Buffer::IsAllocatedOnHost() const
{
  LockType lock = this->Internals->GetLock();
  return (this->Internals->GetHostBuffer(lock).GetPointer() != nullptr);
}

bool Buffer::IsAllocatedOnDevice(vtkm::cont::DeviceAdapterId device) const
{
  if (device.IsValueValid())
  {
    LockType lock = this->Internals->GetLock();
    return (this->Internals->GetDeviceBuffers(lock)[device].GetPointer() != nullptr);
  }
  else if (device == vtkm::cont::DeviceAdapterTagUndefined{})
  {
    // Return if allocated on host.
    return this->IsAllocatedOnHost();
  }
  else if (device == vtkm::cont::DeviceAdapterTagAny{})
  {
    // Return if allocated on any device.
    LockType lock = this->Internals->GetLock();
    for (auto&& deviceBuffer : this->Internals->GetDeviceBuffers(lock))
    {
      if (deviceBuffer.second.GetPointer() != nullptr)
      {
        return true;
      }
    }
    return false;
  }
  else
  {
    // Should we throw an exception here?
    return false;
  }
}

const void* Buffer::ReadPointerHost(vtkm::cont::Token& token) const
{
  LockType lock = this->Internals->GetLock();
  detail::BufferHelper::WaitToRead(this->Internals, lock, token);
  detail::BufferHelper::AllocateOnHost(
    this->Internals, lock, token, detail::BufferHelper::AccessMode::READ);
  return this->Internals->GetHostBuffer(lock).GetPointer();
}

const void* Buffer::ReadPointerDevice(vtkm::cont::DeviceAdapterId device,
                                      vtkm::cont::Token& token) const
{
  if (device.IsValueValid())
  {
    LockType lock = this->Internals->GetLock();
    detail::BufferHelper::WaitToRead(this->Internals, lock, token);
    detail::BufferHelper::AllocateOnDevice(
      this->Internals, lock, token, device, detail::BufferHelper::AccessMode::READ);
    return this->Internals->GetDeviceBuffers(lock)[device].GetPointer();
  }
  else if (device == vtkm::cont::DeviceAdapterTagUndefined{})
  {
    return this->ReadPointerHost(token);
  }
  else
  {
    throw vtkm::cont::ErrorBadDevice("Invalid device given to ReadPointerDevice");
  }
}

void* Buffer::WritePointerHost(vtkm::cont::Token& token) const
{
  LockType lock = this->Internals->GetLock();
  detail::BufferHelper::WaitToWrite(this->Internals, lock, token);
  detail::BufferHelper::AllocateOnHost(
    this->Internals, lock, token, detail::BufferHelper::AccessMode::WRITE);

  // Array is being written on host. All other buffers invalidated, so delete them.
  this->Internals->GetDeviceBuffers(lock).clear();

  return this->Internals->GetHostBuffer(lock).GetPointer();
}

void* Buffer::WritePointerDevice(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token& token) const
{
  if (device.IsValueValid())
  {
    LockType lock = this->Internals->GetLock();
    detail::BufferHelper::WaitToWrite(this->Internals, lock, token);
    detail::BufferHelper::AllocateOnDevice(
      this->Internals, lock, token, device, detail::BufferHelper::AccessMode::WRITE);

    // Array is being written on this device. All other buffers invalided, so delete them.
    this->Internals->GetHostBuffer(lock) = vtkm::cont::internal::BufferInfo{};
    InternalsStruct::DeviceBufferMap& deviceBuffers = this->Internals->GetDeviceBuffers(lock);
    auto iter = deviceBuffers.find(device);
    deviceBuffers.erase(deviceBuffers.begin(), iter);
    iter = deviceBuffers.find(device);
    std::advance(iter, 1);
    deviceBuffers.erase(iter, deviceBuffers.end());

    return this->Internals->GetDeviceBuffers(lock)[device].GetPointer();
  }
  else if (device == vtkm::cont::DeviceAdapterTagUndefined{})
  {
    return this->WritePointerHost(token);
  }
  else
  {
    throw vtkm::cont::ErrorBadDevice("Invalid device given to WritePointerDevice");
  }
}

void Buffer::Enqueue(const vtkm::cont::Token& token) const
{
  LockType lock = this->Internals->GetLock();
  detail::BufferHelper::Enqueue(this->Internals, lock, token);
}

void Buffer::DeepCopy(vtkm::cont::internal::Buffer& dest) const
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType srcLock = this->Internals->GetLock();
    LockType destLock = dest.Internals->GetLock();

    detail::BufferHelper::WaitToRead(this->Internals, srcLock, token);

    // If we are on a device, copy there.
    for (auto&& deviceBuffer : this->Internals->GetDeviceBuffers(srcLock))
    {
      if (deviceBuffer.second.GetPointer() != nullptr)
      {
        detail::BufferHelper::CopyOnDevice(
          deviceBuffer.first, this->Internals, srcLock, dest.Internals, destLock, token);
        return;
      }
    }

    // If we are here, there were no devices to copy on. Copy on host if possible.
    if (this->Internals->GetHostBuffer(srcLock).GetPointer() != nullptr)
    {
      detail::BufferHelper::CopyOnHost(this->Internals, srcLock, dest.Internals, destLock, token);
    }
    else
    {
      // Nothing actually allocated. Just create allocation for dest. (Allocation happens lazily.)
      dest.SetNumberOfBytes(this->GetNumberOfBytes(), vtkm::CopyFlag::Off);
    }
  }
}

void Buffer::DeepCopy(vtkm::cont::internal::Buffer& dest, vtkm::cont::DeviceAdapterId device) const
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType srcLock = this->Internals->GetLock();
    LockType destLock = dest.Internals->GetLock();
    detail::BufferHelper::CopyOnDevice(
      device, this->Internals, srcLock, dest.Internals, destLock, token);
  }
}

void Buffer::Reset(const vtkm::cont::internal::BufferInfo& bufferInfo)
{
  LockType lock = this->Internals->GetLock();

  // Clear out any old buffers.
  this->Internals->GetHostBuffer(lock) = vtkm::cont::internal::BufferInfo{};
  this->Internals->GetDeviceBuffers(lock).clear();

  if (bufferInfo.GetDevice().IsValueValid())
  {
    this->Internals->GetDeviceBuffers(lock)[bufferInfo.GetDevice()] = bufferInfo;
  }
  else if (bufferInfo.GetDevice() == vtkm::cont::DeviceAdapterTagUndefined{})
  {
    this->Internals->GetHostBuffer(lock) = bufferInfo;
  }
  else
  {
    this->Internals->SetNumberOfBytes(lock, 0);
    throw vtkm::cont::ErrorBadDevice("Attempting to reset Buffer to invalid device.");
  }

  this->Internals->SetNumberOfBytes(lock, bufferInfo.GetSize());
}

vtkm::cont::internal::BufferInfo Buffer::GetHostBufferInfo() const
{
  LockType lock = this->Internals->GetLock();
  return this->Internals->GetHostBuffer(lock);
}

vtkm::cont::internal::BufferInfo Buffer::GetDeviceBufferInfo(
  vtkm::cont::DeviceAdapterId device) const
{
  if (device.IsValueValid())
  {
    LockType lock = this->Internals->GetLock();
    return this->Internals->GetDeviceBuffers(lock)[device];
  }
  else if (device == vtkm::cont::DeviceAdapterTagUndefined{})
  {
    return this->GetHostBufferInfo();
  }
  else
  {
    throw vtkm::cont::ErrorBadDevice("Called Buffer::GetDeviceBufferInfo with invalid device");
  }
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
