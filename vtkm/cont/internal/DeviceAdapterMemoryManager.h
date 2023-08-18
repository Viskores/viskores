//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_DeviceAdapterMemoryManager_h
#define vtk_m_cont_internal_DeviceAdapterMemoryManager_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Flags.h>
#include <vtkm/Types.h>

#include <vtkm/cont/DeviceAdapterTag.h>

#include <cstring>
#include <memory>
#include <vector>

namespace vtkm
{

using BufferSizeType = vtkm::Int64;

namespace cont
{
namespace internal
{

struct TransferredBuffer;

namespace detail
{

struct BufferInfoInternals;

} // namespace detail

class VTKM_CONT_EXPORT BufferInfo
{
public:
  /// Returns a pointer to the memory that is allocated. This pointer may only be referenced on
  /// the associated device.
  ///
  VTKM_CONT void* GetPointer() const;

  /// Returns the size of the buffer in bytes.
  ///
  VTKM_CONT vtkm::BufferSizeType GetSize() const;

  /// \brief Returns the device on which this buffer is allocated.
  ///
  /// If the buffer is not on a device (i.e. it is on the host), then DeviceAdapterIdUndefined
  /// is returned.
  ///
  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const;

  VTKM_CONT BufferInfo();
  VTKM_CONT ~BufferInfo();

  VTKM_CONT BufferInfo(const BufferInfo& src);
  VTKM_CONT BufferInfo(BufferInfo&& src);

  VTKM_CONT BufferInfo& operator=(const BufferInfo& src);
  VTKM_CONT BufferInfo& operator=(BufferInfo&& src);

  /// Shallow copy buffer from one host/device to another host/device. Make sure that these
  /// two devices share the same memory space. (This is not checked and will cause badness
  /// if not correct.)
  ///
  VTKM_CONT BufferInfo(const BufferInfo& src, vtkm::cont::DeviceAdapterId device);
  VTKM_CONT BufferInfo(BufferInfo&& src, vtkm::cont::DeviceAdapterId device);

  /// A function callback for deleting the memory.
  ///
  using Deleter = void(void* container);

  /// A function callback for reallocating the memory.
  ///
  using Reallocater = void(void*& memory,
                           void*& container,
                           vtkm::BufferSizeType oldSize,
                           vtkm::BufferSizeType newSize);

  /// Creates a BufferInfo with the given memory, some (unknown) container holding that memory, a
  /// deletion function, and a reallocation function. The deleter will be called with the pointer
  /// to the container when the buffer is released.
  ///
  VTKM_CONT BufferInfo(vtkm::cont::DeviceAdapterId device,
                       void* memory,
                       void* container,
                       vtkm::BufferSizeType size,
                       Deleter deleter,
                       Reallocater reallocater);

  /// Reallocates the buffer to a new size.
  ///
  VTKM_CONT void Reallocate(vtkm::BufferSizeType newSize);

  /// Transfers ownership of the underlying allocation and Deleter and Reallocater to the caller.
  /// After ownership has been transferred this buffer will be equivalant to one that was passed
  /// to VTK-m as `view` only.
  ///
  /// This means that the Deleter will do nothing, and the Reallocater will throw an `ErrorBadAllocation`.
  ///
  VTKM_CONT TransferredBuffer TransferOwnership();

private:
  detail::BufferInfoInternals* Internals;
  vtkm::cont::DeviceAdapterId Device;
};


/// Represents the buffer being transferred to external ownership
///
/// The Memory pointer represents the actual data allocation to
/// be used for access and execution
///
/// The container represents what needs to be deleted. This might
/// not be equivalent to \c Memory when we have transferred things
/// such as std::vector
///
struct TransferredBuffer
{
  void* Memory;
  void* Container;
  BufferInfo::Deleter* Delete;
  BufferInfo::Reallocater* Reallocate;
  vtkm::BufferSizeType Size;
};

/// Allocates a `BufferInfo` object for the host.
///
VTKM_CONT_EXPORT VTKM_CONT vtkm::cont::internal::BufferInfo AllocateOnHost(
  vtkm::BufferSizeType size);

/// \brief The base class for device adapter memory managers.
///
/// Every device adapter is expected to define a specialization of `DeviceAdapterMemoryManager`,
/// and they are all expected to subclass this base class.
///
class VTKM_CONT_EXPORT DeviceAdapterMemoryManagerBase
{
public:
  VTKM_CONT virtual ~DeviceAdapterMemoryManagerBase();

  /// Allocates a buffer of the specified size in bytes and returns a BufferInfo object
  /// containing information about it.
  VTKM_CONT virtual vtkm::cont::internal::BufferInfo Allocate(vtkm::BufferSizeType size) const = 0;

  /// Reallocates the provided buffer to a new size. The passed in `BufferInfo` should be
  /// modified to reflect the changes.
  VTKM_CONT void Reallocate(vtkm::cont::internal::BufferInfo& buffer,
                            vtkm::BufferSizeType newSize) const;

  /// Manages the provided array. Returns a `BufferInfo` object that contains the data.
  VTKM_CONT BufferInfo ManageArray(void* memory,
                                   void* container,
                                   vtkm::BufferSizeType size,
                                   vtkm::cont::internal::BufferInfo::Deleter deleter,
                                   vtkm::cont::internal::BufferInfo::Reallocater reallocater) const;

  /// Returns the device that this manager is associated with.
  VTKM_CONT virtual vtkm::cont::DeviceAdapterId GetDevice() const = 0;

  /// Copies data from the provided host buffer provided onto the device and returns a buffer info
  /// object holding the pointer for the device.
  VTKM_CONT virtual vtkm::cont::internal::BufferInfo CopyHostToDevice(
    const vtkm::cont::internal::BufferInfo& src) const = 0;

  /// Copies data from the provided host buffer into the provided pre-allocated device buffer. The
  /// `BufferInfo` object for the device was created by a previous call to this object.
  VTKM_CONT virtual void CopyHostToDevice(const vtkm::cont::internal::BufferInfo& src,
                                          const vtkm::cont::internal::BufferInfo& dest) const = 0;

  /// Copies data from the device buffer provided to the host. The passed in `BufferInfo` object
  /// was created by a previous call to this object.
  VTKM_CONT virtual vtkm::cont::internal::BufferInfo CopyDeviceToHost(
    const vtkm::cont::internal::BufferInfo& src) const = 0;

  /// Copies data from the device buffer provided into the provided pre-allocated host buffer. The
  /// `BufferInfo` object for the device was created by a previous call to this object.
  VTKM_CONT virtual void CopyDeviceToHost(const vtkm::cont::internal::BufferInfo& src,
                                          const vtkm::cont::internal::BufferInfo& dest) const = 0;

  /// Deep copies data from one device buffer to another device buffer. The passed in `BufferInfo`
  /// object was created by a previous call to this object.
  VTKM_CONT virtual vtkm::cont::internal::BufferInfo CopyDeviceToDevice(
    const vtkm::cont::internal::BufferInfo& src) const = 0;

  /// Deep copies data from one device buffer to another device buffer. The passed in `BufferInfo`
  /// objects were created by a previous call to this object.
  VTKM_CONT virtual void CopyDeviceToDevice(const vtkm::cont::internal::BufferInfo& src,
                                            const vtkm::cont::internal::BufferInfo& dest) const = 0;


  /// \brief Low-level method to allocate memory on the device.
  ///
  /// This method allocates an array of the given number of bytes on the device and returns
  /// a void pointer to the array. The preferred method to allocate memory is to use the
  /// `Allocate` method, which returns a `BufferInfo` that manages its own memory. However,
  /// for cases where you are interfacing with code outside of VTK-m and need just a raw
  /// pointer, this method can be used. The returned memory can be freed with
  /// `DeleteRawPointer`.
  VTKM_CONT virtual void* AllocateRawPointer(vtkm::BufferSizeType size) const;

  /// \brief Low-level method to copy data on the device.
  ///
  /// This method copies data from one raw pointer to another. It performs the same
  /// function as `CopyDeviceToDevice`, except that it operates on raw pointers
  /// instead of `BufferInfo` objects. This is a useful low-level mechanism to move
  /// data on a device in memory locations created externally to VTK-m.
  VTKM_CONT virtual void CopyDeviceToDeviceRawPointer(const void* src,
                                                      void* dest,
                                                      vtkm::BufferSizeType size) const;

  /// \brief Low-level method to delete memory on the device.
  ///
  /// This method takes a pointer to memory allocated on the device and frees it.
  /// The preferred method to delete memory is to use the deallocation routines in
  /// `BufferInfo` objects created with `Allocate`. But for cases where you only
  /// have a raw pointer to the data, this method can be used to manage it. This
  /// method should only be used on memory allocated with this
  /// `DeviceAdaperMemoryManager`.
  VTKM_CONT virtual void DeleteRawPointer(void*) const = 0;
};

/// \brief The device adapter memory manager.
///
/// Every device adapter is expected to define a specialization of `DeviceAdapterMemoryManager`.
/// This class must be a (perhaps indirect) subclass of `DeviceAdapterMemoryManagerBase`. All
/// abstract methods must be implemented.
///
template <typename DeviceAdapterTag>
class DeviceAdapterMemoryManager;

VTKM_CONT_EXPORT VTKM_CONT void HostDeleter(void*);
VTKM_CONT_EXPORT VTKM_CONT void* HostAllocate(vtkm::BufferSizeType);
VTKM_CONT_EXPORT VTKM_CONT void HostReallocate(void*&,
                                               void*&,
                                               vtkm::BufferSizeType,
                                               vtkm::BufferSizeType);


VTKM_CONT_EXPORT VTKM_CONT void InvalidRealloc(void*&,
                                               void*&,
                                               vtkm::BufferSizeType,
                                               vtkm::BufferSizeType);

// Deletes a container object by casting it to a pointer of a given type (the template argument)
// and then using delete[] on the object.
template <typename T>
VTKM_CONT inline void SimpleArrayDeleter(void* container_)
{
  T* container = reinterpret_cast<T*>(container_);
  delete[] container;
}

// Reallocates a standard C array. Note that the allocation method is different than the default
// host allocation of vtkm::cont::internal::BufferInfo and may be less efficient.
template <typename T>
VTKM_CONT inline void SimpleArrayReallocater(void*& memory,
                                             void*& container,
                                             vtkm::BufferSizeType oldSize,
                                             vtkm::BufferSizeType newSize)
{
  VTKM_ASSERT(memory == container);
  VTKM_ASSERT(static_cast<std::size_t>(newSize) % sizeof(T) == 0);

  // If the new size is not much smaller than the old size, just reuse the buffer (and waste a
  // little memory).
  if ((newSize > ((3 * oldSize) / 4)) && (newSize <= oldSize))
  {
    return;
  }

  void* newBuffer = new T[static_cast<std::size_t>(newSize) / sizeof(T)];
  std::memcpy(newBuffer, memory, static_cast<std::size_t>(newSize < oldSize ? newSize : oldSize));

  if (memory != nullptr)
  {
    SimpleArrayDeleter<T>(memory);
  }

  memory = container = newBuffer;
}

// Deletes a container object by casting it to a pointer of a given type (the template argument)
// and then using delete on the object.
template <typename T>
VTKM_CONT inline void CastDeleter(void* container_)
{
  T* container = reinterpret_cast<T*>(container_);
  delete container;
}

template <typename T, typename Allocator>
VTKM_CONT inline void StdVectorDeleter(void* container)
{
  CastDeleter<std::vector<T, Allocator>>(container);
}

template <typename T, typename Allocator>
VTKM_CONT inline void StdVectorReallocater(void*& memory,
                                           void*& container,
                                           vtkm::BufferSizeType oldSize,
                                           vtkm::BufferSizeType newSize)
{
  using vector_type = std::vector<T, Allocator>;
  vector_type* vector = reinterpret_cast<vector_type*>(container);
  VTKM_ASSERT(vector->empty() || (memory == vector->data()));
  VTKM_ASSERT(oldSize == static_cast<vtkm::BufferSizeType>(vector->size() * sizeof(T)));

  vector->resize(static_cast<std::size_t>(newSize));
  memory = vector->data();
}
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_DeviceAdapterMemoryManager_h
