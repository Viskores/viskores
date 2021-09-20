//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>
#include <vtkm/cont/testing/TestingRuntimeDeviceConfiguration.h>

namespace internal = vtkm::cont::internal;

namespace vtkm
{
namespace cont
{
namespace testing
{

template <>
VTKM_CONT void
TestingRuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagTBB>::TestRuntimeConfig()
{
  auto deviceOptions = TestingRuntimeDeviceConfiguration::DefaultInitializeConfigOptions();
  vtkm::Id maxThreads = 0;
  vtkm::Id numThreads = 0;
#if TBB_VERSION_MAJOR >= 2020
  maxThreads = ::tbb::global_control::active_value(::tbb::global_control::max_allowed_parallelism);
  numThreads = maxThreads;
#else
  maxThreads = ::tbb::task_scheduler_init::default_num_threads();
  numThreads = maxThreads;
#endif
  numThreads = numThreads / 2;
  deviceOptions.VTKmNumThreads.SetOption(numThreads);
  auto& config =
    RuntimeDeviceInformation{}.GetRuntimeConfiguration(DeviceAdapterTagTBB(), deviceOptions);
  vtkm::Id setNumThreads;
  vtkm::Id setMaxThreads;
  VTKM_TEST_ASSERT(config.GetThreads(setNumThreads) ==
                     internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                   "Failed to get num threads");
  VTKM_TEST_ASSERT(setNumThreads == numThreads,
                   "RTC's numThreads != numThreads tbb direct! " + std::to_string(setNumThreads) +
                     " != " + std::to_string(numThreads));
  VTKM_TEST_ASSERT(config.GetMaxThreads(setMaxThreads) ==
                     internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                   "Failed to get max threads");
  VTKM_TEST_ASSERT(setMaxThreads == maxThreads,
                   "RTC's maxThreads != maxThreads tbb direct! " + std::to_string(setMaxThreads) +
                     " != " + std::to_string(maxThreads));
}

} // namespace vtkm::cont::testing
} // namespace vtkm::cont
} // namespace vtkm

int UnitTestTBBRuntimeDeviceConfiguration(int argc, char* argv[])
{
  return vtkm::cont::testing::TestingRuntimeDeviceConfiguration<
    vtkm::cont::DeviceAdapterTagTBB>::Run(argc, argv);
}
