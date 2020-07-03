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

#include <memory>

namespace vtkm
{

using BufferSizeType = vtkm::Int64;

namespace cont
{
namespace internal
{

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

private:
  detail::BufferInfoInternals* Internals;
  vtkm::cont::DeviceAdapterId Device;
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
};

/// \brief The device adapter memory manager.
///
/// Every device adapter is expected to define a specialization of `DeviceAdapterMemoryManager`.
/// This class must be a (perhaps indirect) subclass of `DeviceAdapterMemoryManagerBase`. All
/// abstract methods must be implemented.
///
template <typename DeviceAdapterTag>
class DeviceAdapterMemoryManager;
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_DeviceAdapterMemoryManager_h
