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

  VTKM_CONT void Initialize(const RuntimeDeviceConfigurationOptions&) const override final
  {
    // TODO: Initialize threads/numa regions in OpenMP
  }

  VTKM_CONT void SetThreads(const vtkm::Id&) const override final
  {
    // TODO: Set the threads in OpenMP
  }

  VTKM_CONT void SetNumaRegions(const vtkm::Id&) const override final
  {
    // TODO: Set the numa regions in OpenMP
  }

  VTKM_CONT vtkm::Id GetThreads() const override final
  {
    // TODO: Get the number of OpenMP threads
    return 0;
  }

  VTKM_CONT vtkm::Id GetNumaRegions() const override final
  {
    // TODO: Get the number of OpenMP NumaRegions
    return 0;
  }
};
} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif //vtk_m_cont_openmp_internal_RuntimeDeviceConfigurationOpenMP_h
