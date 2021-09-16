//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/openmp/DeviceAdapterOpenMP.h>
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
TestingRuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagOpenMP>::TestRuntimeConfig()
{
  auto deviceOptions = TestingRuntimeDeviceConfiguration::DefaultInitializeConfigOptions();
  vtkm::Id maxThreads = 0;
  vtkm::Id numThreads = 0;
  VTKM_OPENMP_DIRECTIVE(parallel)
  {
    maxThreads = omp_get_max_threads();
    numThreads = omp_get_num_threads();
  }
  VTKM_TEST_ASSERT(maxThreads == numThreads,
                   "openMP by default maxthreads should == numthreads " +
                     std::to_string(maxThreads) + " != " + std::to_string(numThreads));
  numThreads = numThreads / 2;
  deviceOptions.VTKmNumThreads.SetOption(numThreads);
  auto& config =
    RuntimeDeviceInformation{}.GetRuntimeConfiguration(DeviceAdapterTagOpenMP(), deviceOptions);
  vtkm::Id setNumThreads;
  vtkm::Id setMaxThreads;
  VTKM_OPENMP_DIRECTIVE(parallel) { numThreads = omp_get_num_threads(); }
  VTKM_TEST_ASSERT(config.GetThreads(setNumThreads) ==
                     internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                   "Failed to get num threads");
  VTKM_TEST_ASSERT(setNumThreads == numThreads,
                   "RTC's numThreads != numThreads openmp direct! " +
                     std::to_string(setNumThreads) + " != " + std::to_string(numThreads));
  VTKM_TEST_ASSERT(config.GetMaxThreads(setMaxThreads) ==
                     internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                   "Failed to get max threads");
  VTKM_TEST_ASSERT(setMaxThreads == maxThreads,
                   "RTC's maxThreads != maxThreads openmp direct! " +
                     std::to_string(setMaxThreads) + " != " + std::to_string(maxThreads));
}

} // namespace vtkm::cont::testing
} // namespace vtkm::cont
} // namespace vtkm

int UnitTestOpenMPRuntimeDeviceConfiguration(int argc, char* argv[])
{
  return vtkm::cont::testing::TestingRuntimeDeviceConfiguration<
    vtkm::cont::DeviceAdapterTagOpenMP>::Run(argc, argv);
}
