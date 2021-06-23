//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_RuntimeDeviceConfigurationCuda_h
#define vtk_m_cont_cuda_internal_RuntimeDeviceConfigurationCuda_h

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
class RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagCuda>
  : public vtkm::cont::internal::RuntimeDeviceConfigurationBase
{
  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const override final
  {
    return vtkm::cont::DeviceAdapterTagCuda{};
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetDeviceInstance(
    const vtkm::Id&) const override final
  {
    // TODO: set the cuda device instance
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetDeviceInstance(vtkm::Id&) const override final
  {
    // TODO: Get the cuda device instance (also maybe a list of available devices?)
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }
};
} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif //vtk_m_cont_cuda_internal_RuntimeDeviceConfigurationCuda_h
