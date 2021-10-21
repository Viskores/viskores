//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_tbb_internal_RuntimeDeviceConfigurationTBB_h
#define vtk_m_cont_tbb_internal_RuntimeDeviceConfigurationTBB_h

#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#if TBB_VERSION_MAJOR >= 2020
#define TBB_PREVIEW_GLOBAL_CONTROL
#include <tbb/global_control.h>
#include <tbb/task_arena.h>
#else
#include <tbb/tbb.h>
#endif
VTKM_THIRDPARTY_POST_INCLUDE

#include <memory>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
class RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagTBB>
  : public vtkm::cont::internal::RuntimeDeviceConfigurationBase
{
public:
  VTKM_CONT
  RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagTBB>()
    :
#if TBB_VERSION_MAJOR >= 2020
    HardwareMaxThreads(::tbb::task_arena{}.max_concurrency())
    ,
#else
    HardwareMaxThreads(::tbb::task_scheduler_init::default_num_threads())
    ,
#endif
    CurrentNumThreads(this->HardwareMaxThreads)
  {
  }

  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const override final
  {
    return vtkm::cont::DeviceAdapterTagTBB{};
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetThreads(const vtkm::Id& value) override final
  {
    this->CurrentNumThreads = value > 0 ? value : this->HardwareMaxThreads;
#if TBB_VERSION_MAJOR >= 2020
    GlobalControl.reset(new ::tbb::global_control(::tbb::global_control::max_allowed_parallelism,
                                                  this->CurrentNumThreads));
#else
    TaskSchedulerInit.reset(
      new ::tbb::task_scheduler_init(static_cast<int>(this->CurrentNumThreads)));
#endif
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetThreads(vtkm::Id& value) const override final
  {
#if TBB_VERSION_MAJOR >= 2020
    value = ::tbb::global_control::active_value(::tbb::global_control::max_allowed_parallelism);
#else
    value = this->CurrentNumThreads;
#endif
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetMaxThreads(
    vtkm::Id& value) const override final
  {
    value = this->HardwareMaxThreads;
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

private:
#if TBB_VERSION_MAJOR >= 2020
  std::unique_ptr<::tbb::global_control> GlobalControl;
#else
  std::unique_ptr<::tbb::task_scheduler_init> TaskSchedulerInit;
#endif
  vtkm::Id HardwareMaxThreads;
  vtkm::Id CurrentNumThreads;
};
} // namespace vktm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif //vtk_m_cont_tbb_internal_RuntimeDeviceConfigurationTBB_h
