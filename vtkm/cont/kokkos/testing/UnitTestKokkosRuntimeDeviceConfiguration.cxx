//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/kokkos/DeviceAdapterKokkos.h>
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
TestingRuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagKokkos>::TestRuntimeConfig()
{
  int argc;
  char** argv;
  vtkm::cont::testing::Testing::MakeArgs(argc, argv, "--kokkos-print-configuration");
  auto deviceOptions = TestingRuntimeDeviceConfiguration::DefaultInitializeConfigOptions();
  deviceOptions.VTKmDeviceInstance.SetOption(0);
  internal::RuntimeDeviceConfigurationBase& config =
    RuntimeDeviceInformation{}.GetRuntimeConfiguration(
      DeviceAdapterTagKokkos(), deviceOptions, argc, argv);
  VTKM_TEST_ASSERT(Kokkos::is_initialized(), "Kokkos should be initialized at this point");

  // Test that args are set and the right arg priority is applied
  vtkm::Id testValue;
  VTKM_TEST_ASSERT(config.GetThreads(testValue) == internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                   "Failed to get set threads");
  VTKM_TEST_ASSERT(testValue == 8,
                   "Set threads does not match expected value: 8 != " + std::to_string(testValue));
  VTKM_TEST_ASSERT(config.GetDeviceInstance(testValue) ==
                     internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                   "Failed to get set device instance");
  VTKM_TEST_ASSERT(testValue == 0,
                   "Set device instance does not match expected value: 0 != " +
                     std::to_string(testValue));
  std::cout
    << "Ensure that with kokkos we can't re-initialize or set values after the first initialize"
    << std::endl;
  std::cout << "This should pop up a few warnings in the test logs" << std::endl;
  deviceOptions.VTKmNumThreads.SetOption(16);
  deviceOptions.VTKmDeviceInstance.SetOption(5);
  config.Initialize(deviceOptions);
  VTKM_TEST_ASSERT(config.SetThreads(1) == internal::RuntimeDeviceConfigReturnCode::NOT_APPLIED,
                   "Shouldn't be able to set threads after kokkos is initalized");
  VTKM_TEST_ASSERT(config.SetDeviceInstance(1) ==
                     internal::RuntimeDeviceConfigReturnCode::NOT_APPLIED,
                   "Shouldn't be able to set device instnace after kokkos is initalized");

  // make sure all the values are the same
  VTKM_TEST_ASSERT(config.GetThreads(testValue) == internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                   "Failed to get set threads");
  VTKM_TEST_ASSERT(testValue == 8,
                   "Set threads does not match expected value: 8 != " + std::to_string(testValue));
  VTKM_TEST_ASSERT(config.GetDeviceInstance(testValue) ==
                     internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                   "Failed to get set device instance");
  VTKM_TEST_ASSERT(testValue == 0,
                   "Set device instance does not match expected value: 0 != " +
                     std::to_string(testValue));
}

} // namespace vtkm::cont::testing
} // namespace vtkm::cont
} // namespace vtkm

int UnitTestKokkosRuntimeDeviceConfiguration(int argc, char* argv[])
{
  return vtkm::cont::testing::TestingRuntimeDeviceConfiguration<
    vtkm::cont::DeviceAdapterTagKokkos>::Run(argc, argv);
}
