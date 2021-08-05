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

#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorBadDevice.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>

#include <vtkm/cont/internal/Buffer.h>
#include <vtkm/cont/internal/DeviceAdapterMemoryManager.h>

#include <condition_variable>
#include <cstring>
#include <deque>
#include <map>
#include <memory>

namespace vtkm
{
namespace internal
{

vtkm::BufferSizeType NumberOfValuesToNumberOfBytes(vtkm::Id numValues, std::size_t typeSize)
{
  VTKM_ASSERT(numValues >= 0);

  if (numValues > (std::numeric_limits<vtkm::BufferSizeType>::max() /
                   static_cast<vtkm::BufferSizeType>(typeSize)))
  {
    throw vtkm::cont::ErrorBadAllocation("Asking for a buffer too big to represent.");
  }

  return numValues * static_cast<vtkm::BufferSizeType>(typeSize);
}
}
} // namespace vtkm::internal

namespace
{

using LockType = std::unique_lock<std::mutex>;

struct BufferState
{
  vtkm::cont::internal::BufferInfo Info;
  bool Pinned = false;
  bool UpToDate = false;

  BufferState() = default;
  BufferState(const vtkm::cont::internal::BufferInfo& info,
              bool pinned = false,
              bool upToDate = true)
    : Info(info)
    , Pinned(pinned)
    , UpToDate(upToDate)
  {
  }

  // Automatically convert to BufferInfo
  operator vtkm::cont::internal::BufferInfo&() { return this->Info; }
  operator const vtkm::cont::internal::BufferInfo&() const { return this->Info; }

  // Pass through common BufferInfo methods.

  void* GetPointer() const { return this->Info.GetPointer(); }

  vtkm::BufferSizeType GetSize() const { return this->Info.GetSize(); }

  vtkm::cont::DeviceAdapterId GetDevice() const { return this->Info.GetDevice(); }

  void Reallocate(vtkm::BufferSizeType newSize)
  {
    if (this->Info.GetSize() != newSize)
    {
      if (this->Pinned)
      {
        throw vtkm::cont::ErrorBadAllocation("Attempted to reallocate a pinned buffer.");
      }
      this->Info.Reallocate(newSize);
    }
  }

  /// Releases the buffer. If the memory is not pinned, it is deleted. In any case, it is
  /// marked as no longer up to date.
  void Release()
  {
    if (!this->Pinned)
    {
      this->Info = vtkm::cont::internal::BufferInfo{};
    }
    this->UpToDate = false;
  }
};

struct MetaDataManager
{
  void* Data = nullptr;
  std::string Type;
  vtkm::cont::internal::detail::DeleterType* Deleter = nullptr;
  vtkm::cont::internal::detail::CopierType* Copier = nullptr;

  MetaDataManager() = default;

  ~MetaDataManager()
  {
    if (this->Data != nullptr)
    {
      VTKM_ASSERT(this->Deleter != nullptr);
      this->Deleter(this->Data);
      this->Data = nullptr;
    }
  }

  // We don't know how much information is the metadata, and copying it could be expensive.
  // Thus, we want to be intentional about copying the metadata only for deep copies.
  MetaDataManager(const MetaDataManager& src) = delete;
  MetaDataManager& operator=(const MetaDataManager& src) = delete;

  void Initialize(void* data,
                  const std::string& type,
                  vtkm::cont::internal::detail::DeleterType* deleter,
                  vtkm::cont::internal::detail::CopierType* copier)
  {
    Data = data;
    Type = type;
    Deleter = deleter;
    Copier = copier;
  }

  void DeepCopyFrom(const MetaDataManager& src)
  {
    if (this->Data != nullptr)
    {
      VTKM_ASSERT(this->Deleter != nullptr);
      this->Deleter(this->Data);
      this->Data = nullptr;
      this->Type = "";
    }
    if (src.Data != nullptr)
    {
      VTKM_ASSERT(src.Copier);
      VTKM_ASSERT(src.Deleter);
      this->Data = src.Copier(src.Data);
      this->Type = src.Type;
      this->Deleter = src.Deleter;
      this->Copier = src.Copier;
    }
  }
};

} // anonymous namespace

namespace vtkm
{
namespace cont
{
namespace internal
{

class Buffer::InternalsStruct
{
public:
  using DeviceBufferMap = std::map<vtkm::cont::DeviceAdapterId, BufferState>;

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
  BufferState HostBuffer;

public:
  std::mutex Mutex;
  std::condition_variable ConditionVariable;

  MetaDataManager MetaData;

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

  VTKM_CONT BufferState& GetHostBuffer(const LockType& lock)
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

  static void SetNumberOfBytes(const std::shared_ptr<Buffer::InternalsStruct>& internals,
                               std::unique_lock<std::mutex>& lock,
                               vtkm::BufferSizeType numberOfBytes,
                               vtkm::CopyFlag preserve,
                               vtkm::cont::Token& token)
  {
    VTKM_ASSUME(numberOfBytes >= 0);

    if (internals->GetNumberOfBytes(lock) == numberOfBytes)
    {
      // Allocation has not changed. Just return.
      // Note, if you set the size to the old size and then try to get the buffer on a different
      // place, a copy might happen.
      return;
    }

    // We are altering the array, so make sure we can write to it.
    BufferHelper::WaitToWrite(internals, lock, token);

    internals->SetNumberOfBytes(lock, numberOfBytes);
    if ((preserve == vtkm::CopyFlag::Off) || (numberOfBytes == 0))
    {
      // No longer need these buffers. Just release them.
      internals->GetHostBuffer(lock).Release();
      for (auto&& deviceBuffer : internals->GetDeviceBuffers(lock))
      {
        deviceBuffer.second.Release();
      }
    }
    else
    {
      // Do nothing (other than resetting numberOfBytes). Buffers will get resized when you get the
      // pointer.
    }
  }

  static void AllocateOnHost(const std::shared_ptr<Buffer::InternalsStruct>& internals,
                             std::unique_lock<std::mutex>& lock,
                             vtkm::cont::Token& token,
                             AccessMode accessMode)
  {
    Wait(internals, lock, token, accessMode);
    BufferState& hostBuffer = internals->GetHostBuffer(lock);
    vtkm::BufferSizeType targetSize = internals->GetNumberOfBytes(lock);
    if (hostBuffer.UpToDate)
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
      if (!deviceBuffer.second.UpToDate)
      {
        continue;
      }

      vtkm::cont::internal::DeviceAdapterMemoryManagerBase& memoryManager =
        vtkm::cont::RuntimeDeviceInformation().GetMemoryManager(deviceBuffer.first);

      if (deviceBuffer.second.GetSize() > targetSize)
      {
        // Device buffer too large. Resize.
        deviceBuffer.second.Reallocate(targetSize);
      }

      if (!hostBuffer.Pinned)
      {
        hostBuffer = memoryManager.CopyDeviceToHost(deviceBuffer.second);
      }
      else
      {
        hostBuffer.Reallocate(targetSize);
        memoryManager.CopyDeviceToHost(deviceBuffer.second, hostBuffer);
      }

      if (hostBuffer.GetSize() != targetSize)
      {
        hostBuffer.Reallocate(targetSize);
      }

      hostBuffer.UpToDate = true;

      return;
    }

    // Buffer not up to date on host or any device, so just allocate a buffer.
    if (!hostBuffer.Pinned)
    {
      hostBuffer = vtkm::cont::internal::AllocateOnHost(targetSize);
    }
    else
    {
      hostBuffer.Reallocate(targetSize);
      hostBuffer.UpToDate = true;
    }
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

    if (deviceBuffers[device].UpToDate)
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
    if (!internals->GetHostBuffer(lock).UpToDate)
    {
      for (auto&& deviceBuffer : deviceBuffers)
      {
        if (deviceBuffer.second.UpToDate)
        {
          // Copy data to host.
          AllocateOnHost(internals, lock, token, accessMode);
          break;
        }
      }
    }

    // If the buffer is now on the host, copy it to the device.
    BufferState& hostBuffer = internals->GetHostBuffer(lock);
    if (hostBuffer.UpToDate)
    {
      if (hostBuffer.GetSize() > targetSize)
      {
        // Host buffer too large. Resize.
        hostBuffer.Reallocate(targetSize);
      }

      if (!deviceBuffers[device].Pinned)
      {
        deviceBuffers[device] = memoryManager.CopyHostToDevice(hostBuffer);
      }
      else
      {
        deviceBuffers[device].Reallocate(targetSize);
        memoryManager.CopyHostToDevice(hostBuffer, deviceBuffers[device]);
      }

      if (deviceBuffers[device].GetSize() != targetSize)
      {
        deviceBuffers[device].Reallocate(targetSize);
      }
      VTKM_ASSERT(deviceBuffers[device].GetSize() == targetSize);

      deviceBuffers[device].UpToDate = true;

      return;
    }

    // Buffer not up to date anywhere, so just allocate a buffer.
    if (!deviceBuffers[device].Pinned)
    {
      deviceBuffers[device] = memoryManager.Allocate(targetSize);
    }
    else
    {
      deviceBuffers[device].Reallocate(targetSize);
      deviceBuffers[device].UpToDate = true;
    }
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

    vtkm::BufferSizeType size = srcInternals->GetNumberOfBytes(srcLock);

    // Any current buffers in destination can be (and should be) deleted.
    // Do this before allocating on the host to avoid unnecessary data copies.
    for (auto&& deviceBuffer : destInternals->GetDeviceBuffers(destLock))
    {
      deviceBuffer.second.Release();
    }

    destInternals->SetNumberOfBytes(destLock, size);
    AllocateOnHost(destInternals, destLock, token, AccessMode::WRITE);

    AllocateOnHost(srcInternals, srcLock, token, AccessMode::READ);

    std::memcpy(destInternals->GetHostBuffer(destLock).GetPointer(),
                srcInternals->GetHostBuffer(srcLock).GetPointer(),
                static_cast<std::size_t>(size));

    destInternals->MetaData.DeepCopyFrom(srcInternals->MetaData);
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
    // Do this before allocating on the host to avoid unnecessary data copies.
    vtkm::cont::internal::Buffer::InternalsStruct::DeviceBufferMap& destDeviceBuffers =
      destInternals->GetDeviceBuffers(destLock);
    destInternals->GetHostBuffer(destLock).Release();
    for (auto&& deviceBuffer : destDeviceBuffers)
    {
      deviceBuffer.second.Release();
    }

    // Do the copy
    vtkm::cont::internal::DeviceAdapterMemoryManagerBase& memoryManager =
      vtkm::cont::RuntimeDeviceInformation().GetMemoryManager(device);

    if (!destDeviceBuffers[device].Pinned)
    {
      destDeviceBuffers[device] =
        memoryManager.CopyDeviceToDevice(srcInternals->GetDeviceBuffers(srcLock)[device]);
    }
    else
    {
      memoryManager.CopyDeviceToDevice(srcInternals->GetDeviceBuffers(srcLock)[device],
                                       destDeviceBuffers[device]);
      destDeviceBuffers[device].UpToDate = true;
    }

    destInternals->SetNumberOfBytes(destLock, srcInternals->GetNumberOfBytes(srcLock));

    destInternals->MetaData.DeepCopyFrom(srcInternals->MetaData);
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
Buffer::Buffer(Buffer&& src) noexcept
  : Internals(std::move(src.Internals))
{
}

// Defined to prevent issues with CUDA
Buffer::~Buffer() {}

// Defined to prevent issues with CUDA
Buffer& Buffer::operator=(const Buffer& src)
{
  this->Internals = src.Internals;
  return *this;
}

// Defined to prevent issues with CUDA
Buffer& Buffer::operator=(Buffer&& src) noexcept
{
  this->Internals = std::move(src.Internals);
  return *this;
}

vtkm::BufferSizeType Buffer::GetNumberOfBytes() const
{
  LockType lock = this->Internals->GetLock();
  return this->Internals->GetNumberOfBytes(lock);
}

void Buffer::SetNumberOfBytes(vtkm::BufferSizeType numberOfBytes,
                              vtkm::CopyFlag preserve,
                              vtkm::cont::Token& token)
{
  LockType lock = this->Internals->GetLock();
  detail::BufferHelper::SetNumberOfBytes(this->Internals, lock, numberOfBytes, preserve, token);
}

bool Buffer::HasMetaData() const
{
  return (this->Internals->MetaData.Data != nullptr);
}

bool Buffer::MetaDataIsType(const std::string& type) const
{
  return this->HasMetaData() && (this->Internals->MetaData.Type == type);
}

void Buffer::SetMetaData(void* data,
                         const std::string& type,
                         detail::DeleterType* deleter,
                         detail::CopierType* copier) const
{
  this->Internals->MetaData.Initialize(data, type, deleter, copier);
}

void* Buffer::GetMetaData(const std::string& type) const
{
  if (type != this->Internals->MetaData.Type)
  {
    throw vtkm::cont::ErrorBadType("Requesting Buffer meta data that is the wrong type.");
  }
  return this->Internals->MetaData.Data;
}

bool Buffer::IsAllocatedOnHost() const
{
  LockType lock = this->Internals->GetLock();
  if (this->Internals->GetNumberOfBytes(lock) > 0)
  {
    return this->Internals->GetHostBuffer(lock).UpToDate;
  }
  else
  {
    // Nothing allocated. Say the data exists everywhere.
    return true;
  }
}

bool Buffer::IsAllocatedOnDevice(vtkm::cont::DeviceAdapterId device) const
{
  if (device.IsValueValid())
  {
    LockType lock = this->Internals->GetLock();
    if (this->Internals->GetNumberOfBytes(lock) > 0)
    {
      return this->Internals->GetDeviceBuffers(lock)[device].UpToDate;
    }
    else
    {
      // Nothing allocated. Say the data exists everywhere.
      return true;
    }
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
    if (this->Internals->GetNumberOfBytes(lock) <= 0)
    {
      // Nothing allocated. Say the data exists everywhere.
      return true;
    }
    for (auto&& deviceBuffer : this->Internals->GetDeviceBuffers(lock))
    {
      if (deviceBuffer.second.UpToDate)
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
  for (auto&& deviceBuffer : this->Internals->GetDeviceBuffers(lock))
  {
    deviceBuffer.second.Release();
  }

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
    this->Internals->GetHostBuffer(lock).Release();
    for (auto&& deviceBuffer : this->Internals->GetDeviceBuffers(lock))
    {
      if (deviceBuffer.first != device)
      {
        deviceBuffer.second.Release();
      }
    }

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

void Buffer::DeepCopyFrom(const vtkm::cont::internal::Buffer& src) const
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    const vtkm::cont::internal::Buffer& dest = *this;

    LockType srcLock = src.Internals->GetLock();
    LockType destLock = dest.Internals->GetLock();

    detail::BufferHelper::WaitToRead(src.Internals, srcLock, token);

    // If we are on a device, copy there.
    for (auto&& deviceBuffer : src.Internals->GetDeviceBuffers(srcLock))
    {
      if (deviceBuffer.second.UpToDate)
      {
        detail::BufferHelper::CopyOnDevice(
          deviceBuffer.first, src.Internals, srcLock, dest.Internals, destLock, token);
        return;
      }
    }

    // If we are here, there were no devices to copy on. Copy on host if possible.
    if (src.Internals->GetHostBuffer(srcLock).UpToDate)
    {
      detail::BufferHelper::CopyOnHost(src.Internals, srcLock, dest.Internals, destLock, token);
    }
    else
    {
      // Nothing actually allocated. Just create allocation for dest. (Allocation happens lazily.)
      detail::BufferHelper::SetNumberOfBytes(dest.Internals,
                                             destLock,
                                             src.Internals->GetNumberOfBytes(srcLock),
                                             vtkm::CopyFlag::Off,
                                             token);
      dest.Internals->MetaData.DeepCopyFrom(src.Internals->MetaData);
    }
  }
}

void Buffer::DeepCopyFrom(const vtkm::cont::internal::Buffer& src,
                          vtkm::cont::DeviceAdapterId device) const
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType srcLock = src.Internals->GetLock();
    LockType destLock = this->Internals->GetLock();
    detail::BufferHelper::CopyOnDevice(
      device, this->Internals, srcLock, this->Internals, destLock, token);
  }
}

void Buffer::Reset(const vtkm::cont::internal::BufferInfo& bufferInfo)
{
  LockType lock = this->Internals->GetLock();

  // Clear out any old buffers. Because we are resetting the object, we will also get rid of
  // pinned memory.
  this->Internals->GetHostBuffer(lock) = BufferState{};
  this->Internals->GetDeviceBuffers(lock).clear();

  if (bufferInfo.GetDevice().IsValueValid())
  {
    this->Internals->GetDeviceBuffers(lock)[bufferInfo.GetDevice()] =
      BufferState{ bufferInfo, true, true };
  }
  else if (bufferInfo.GetDevice() == vtkm::cont::DeviceAdapterTagUndefined{})
  {
    this->Internals->GetHostBuffer(lock) = BufferState{ bufferInfo, true, true };
  }
  else
  {
    this->Internals->SetNumberOfBytes(lock, 0);
    throw vtkm::cont::ErrorBadDevice("Attempting to reset Buffer to invalid device.");
  }

  this->Internals->SetNumberOfBytes(lock, bufferInfo.GetSize());
}

void Buffer::ReleaseDeviceResources() const
{
  vtkm::cont::Token token;

  // Getting a write host buffer will invalidate any device arrays and preserve data
  // on the host (copying if necessary).
  this->WritePointerHost(token);
}

vtkm::cont::internal::BufferInfo Buffer::GetHostBufferInfo() const
{
  LockType lock = this->Internals->GetLock();
  return this->Internals->GetHostBuffer(lock);
}

vtkm::cont::internal::TransferredBuffer Buffer::TakeHostBufferOwnership()
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to acquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->Internals->GetLock();
    detail::BufferHelper::AllocateOnHost(
      this->Internals, lock, token, detail::BufferHelper::AccessMode::READ);
    auto& buffer = this->Internals->GetHostBuffer(lock);
    buffer.Pinned = true;
    return buffer.Info.TransferOwnership();
  }
}

vtkm::cont::internal::TransferredBuffer Buffer::TakeDeviceBufferOwnership(
  vtkm::cont::DeviceAdapterId device)
{
  if (device.IsValueValid())
  {
    // A Token should not be declared within the scope of a lock. when the token goes out of scope
    // it will attempt to acquire the lock, which is undefined behavior of the thread already has
    // the lock.
    vtkm::cont::Token token;
    {
      LockType lock = this->Internals->GetLock();
      detail::BufferHelper::AllocateOnDevice(
        this->Internals, lock, token, device, detail::BufferHelper::AccessMode::READ);
      auto& buffer = this->Internals->GetDeviceBuffers(lock)[device];
      buffer.Pinned = true;
      return buffer.Info.TransferOwnership();
    }
  }
  else if (device == vtkm::cont::DeviceAdapterTagUndefined{})
  {
    return this->TakeHostBufferOwnership();
  }
  else
  {
    throw vtkm::cont::ErrorBadDevice(
      "Called Buffer::TakeDeviceBufferOwnership with invalid device");
  }
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

  vtkm::cont::Token token;
  obj.SetNumberOfBytes(size, vtkm::CopyFlag::Off, token);
  vtkm::UInt8* data = reinterpret_cast<vtkm::UInt8*>(obj.WritePointerHost(token));
  vtkmdiy::load(bb, data, static_cast<std::size_t>(size));
}

} // namespace diy
