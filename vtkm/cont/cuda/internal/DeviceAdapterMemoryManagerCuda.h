//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterMemoryManagerCuda_h
#define vtk_m_cont_cuda_internal_DeviceAdapterMemoryManagerCuda_h

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>

#include <vtkm/cont/internal/DeviceAdapterMemoryManager.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
class VTKM_CONT_EXPORT DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagCuda>
  : public DeviceAdapterMemoryManagerBase
{
public:
  VTKM_CONT vtkm::cont::internal::BufferInfo Allocate(vtkm::BufferSizeType size) const override;

  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const override;

  VTKM_CONT vtkm::cont::internal::BufferInfo CopyHostToDevice(
    const vtkm::cont::internal::BufferInfo& src) const override;

  VTKM_CONT virtual void CopyHostToDevice(
    const vtkm::cont::internal::BufferInfo& src,
    const vtkm::cont::internal::BufferInfo& dest) const override;

  VTKM_CONT vtkm::cont::internal::BufferInfo CopyDeviceToHost(
    const vtkm::cont::internal::BufferInfo& src) const override;

  VTKM_CONT virtual void CopyDeviceToHost(
    const vtkm::cont::internal::BufferInfo& src,
    const vtkm::cont::internal::BufferInfo& dest) const override;

  VTKM_CONT vtkm::cont::internal::BufferInfo CopyDeviceToDevice(
    const vtkm::cont::internal::BufferInfo& src) const override;

  VTKM_CONT virtual void CopyDeviceToDevice(
    const vtkm::cont::internal::BufferInfo& src,
    const vtkm::cont::internal::BufferInfo& dest) const override;
};
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_cuda_internal_DeviceAdapterMemoryManagerCuda_h
