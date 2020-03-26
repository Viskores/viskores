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

class VTKM_CONT_EXPORT BufferInfo
{
public:
  /// Returns a pointer to the memory that is allocated. This pointer may only be referenced on
  /// the associated device.
  ///
  VTKM_CONT virtual void* GetPointer() const = 0;

  /// Returns the size of the buffer in bytes.
  ///
  VTKM_CONT virtual vtkm::BufferSizeType GetSize() const = 0;

  VTKM_CONT virtual ~BufferInfo();

protected:
  BufferInfo() = default;
};

class VTKM_CONT_EXPORT BufferInfoHost : public BufferInfo
{
  std::shared_ptr<vtkm::UInt8> Buffer;
  vtkm::BufferSizeType Size;

public:
  VTKM_CONT BufferInfoHost();
  VTKM_CONT BufferInfoHost(const std::shared_ptr<vtkm::UInt8>& buffer, vtkm::BufferSizeType size);
  VTKM_CONT BufferInfoHost(vtkm::BufferSizeType size);

  VTKM_CONT virtual void* GetPointer() const override;

  VTKM_CONT virtual vtkm::BufferSizeType GetSize() const override;

  VTKM_CONT void Allocate(vtkm::BufferSizeType size, vtkm::CopyFlag preserve);

  VTKM_CONT std::shared_ptr<vtkm::UInt8> GetSharedPointer() const;
};

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
  VTKM_CONT virtual std::shared_ptr<BufferInfo> Allocate(vtkm::BufferSizeType size) = 0;

  /// Manages the provided array. Returns a `BufferInfo` object that contains the data.
  /// The deleter in the `shared_ptr` is expected to correctly free the data.
  VTKM_CONT virtual std::shared_ptr<BufferInfo> ManageArray(std::shared_ptr<vtkm::UInt8> buffer,
                                                            vtkm::BufferSizeType size) = 0;

  /// Reallocates the provided buffer to a new size. The passed in `BufferInfo` should be
  /// modified to reflect the changes.
  VTKM_CONT virtual void Reallocate(std::shared_ptr<vtkm::cont::internal::BufferInfo> buffer,
                                    vtkm::BufferSizeType newSize) = 0;

  /// Copies data from the host buffer provided onto the device and returns a buffer info
  /// object holding the pointer for the device.
  VTKM_CONT virtual std::shared_ptr<vtkm::cont::internal::BufferInfo> CopyHostToDevice(
    std::shared_ptr<vtkm::cont::internal::BufferInfoHost> src) = 0;

  /// Copies data from the device buffer provided to the host. The passed in `BufferInfo` object
  /// was created by a previous call to this object.
  VTKM_CONT virtual std::shared_ptr<vtkm::cont::internal::BufferInfoHost> CopyDeviceToHost(
    std::shared_ptr<vtkm::cont::internal::BufferInfo> src) = 0;

  /// Copies data from one device buffer to another device buffer. The passed in `BufferInfo` object
  /// was created by a previous call to this object.
  VTKM_CONT virtual std::shared_ptr<vtkm::cont::internal::BufferInfo> CopyDeviceToDevice(
    std::shared_ptr<vtkm::cont::internal::BufferInfo> src) = 0;
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
