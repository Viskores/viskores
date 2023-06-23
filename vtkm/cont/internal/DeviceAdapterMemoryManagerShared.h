//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_DeviceAdapterMemoryManagerShared_h
#define vtk_m_cont_internal_DeviceAdapterMemoryManagerShared_h

#include <vtkm/cont/internal/DeviceAdapterMemoryManager.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// \brief An implementation of DeviceAdapterMemoryManager for devices that share memory with the
/// host.
///
/// Device adapters that have shared memory with the host can implement their
/// `DeviceAdapterMemoryManager` as a simple subclass of this.
///
class VTKM_CONT_EXPORT DeviceAdapterMemoryManagerShared : public DeviceAdapterMemoryManagerBase
{
public:
  VTKM_CONT BufferInfo Allocate(vtkm::BufferSizeType size) const override;

  VTKM_CONT virtual vtkm::cont::internal::BufferInfo CopyHostToDevice(
    const vtkm::cont::internal::BufferInfo& src) const override;

  VTKM_CONT virtual void CopyHostToDevice(
    const vtkm::cont::internal::BufferInfo& src,
    const vtkm::cont::internal::BufferInfo& dest) const override;

  VTKM_CONT virtual vtkm::cont::internal::BufferInfo CopyDeviceToHost(
    const vtkm::cont::internal::BufferInfo& src) const override;

  VTKM_CONT virtual void CopyDeviceToHost(
    const vtkm::cont::internal::BufferInfo& src,
    const vtkm::cont::internal::BufferInfo& dest) const override;

  VTKM_CONT virtual vtkm::cont::internal::BufferInfo CopyDeviceToDevice(
    const vtkm::cont::internal::BufferInfo& src) const override;

  VTKM_CONT virtual void CopyDeviceToDevice(
    const vtkm::cont::internal::BufferInfo& src,
    const vtkm::cont::internal::BufferInfo& dest) const override;

  VTKM_CONT virtual void DeleteRawPointer(void* mem) const override;
};
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_DeviceAdapterMemoryManagerShared_h
