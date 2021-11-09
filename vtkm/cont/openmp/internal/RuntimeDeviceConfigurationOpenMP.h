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

#include <vtkm/cont/Logging.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <omp.h>
VTKM_THIRDPARTY_POST_INCLUDE

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
public:
  RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagOpenMP>()
    : HardwareMaxThreads(InitializeHardwareMaxThreads())
    , CurrentNumThreads(this->HardwareMaxThreads)
  {
  }

  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const override final
  {
    return vtkm::cont::DeviceAdapterTagOpenMP{};
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetThreads(const vtkm::Id& value) override final
  {
    if (omp_in_parallel())
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Error, "OpenMP SetThreads: Error, currently in parallel");
      return RuntimeDeviceConfigReturnCode::NOT_APPLIED;
    }
    if (value > 0)
    {
      if (value > this->HardwareMaxThreads)
      {
        VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                   "OpenMP: You may be oversubscribing your CPU cores: "
                     << "process threads available: " << this->HardwareMaxThreads
                     << ", requested threads: " << value);
      }
      this->CurrentNumThreads = value;
      omp_set_num_threads(this->CurrentNumThreads);
    }
    else
    {
      this->CurrentNumThreads = this->HardwareMaxThreads;
      omp_set_num_threads(this->CurrentNumThreads);
    }
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetThreads(vtkm::Id& value) const override final
  {
    value = this->CurrentNumThreads;
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetMaxThreads(
    vtkm::Id& value) const override final
  {
    value = this->HardwareMaxThreads;
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

private:
  VTKM_CONT vtkm::Id InitializeHardwareMaxThreads() const
  {
    vtkm::Id count = 0;

    if (omp_in_parallel())
    {
      count = omp_get_num_threads();
    }
    else
    {
      VTKM_OPENMP_DIRECTIVE(parallel)
      {
        VTKM_OPENMP_DIRECTIVE(atomic)
        ++count;
      }
    }
    return count;
  }

  vtkm::Id HardwareMaxThreads;
  vtkm::Id CurrentNumThreads;
};
} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif //vtk_m_cont_openmp_internal_RuntimeDeviceConfigurationOpenMP_h
