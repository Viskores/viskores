//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_openmp_internal_RuntimeDeviceConfigurationOpenMP_h
#define vtk_m_cont_openmp_internal_RuntimeDeviceConfigurationOpenMP_h

#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>
#include <vtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
class RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagOpenMP>
  : public vtkm::cont::internal::RuntimeDeviceConfigurationBase
{
  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const override final
  {
    return vtkm::cont::DeviceAdapterTagOpenMP{};
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetThreads(const vtkm::Id&) override final
  {
    // TODO: Set the threads in OpenMP
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetNumaRegions(const vtkm::Id&) override final
  {
    // TODO: Set the numa regions in OpenMP
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetThreads(vtkm::Id&) const override final
  {
    // TODO: Get the number of OpenMP threads
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetNumaRegions(vtkm::Id&) const override final
  {
    // TODO: Get the number of OpenMP NumaRegions
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetMaxThreads(vtkm::Id&) const override final
  {
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }
};
} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif //vtk_m_cont_openmp_internal_RuntimeDeviceConfigurationOpenMP_h
