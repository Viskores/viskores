//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingRuntimeDeviceConfiguration_h
#define vtk_m_cont_testing_TestingRuntimeDeviceConfiguration_h

#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>
#include <vtkm/cont/internal/RuntimeDeviceConfigurationOptions.h>
#include <vtkm/cont/testing/Testing.h>

namespace internal = vtkm::cont::internal;

namespace vtkm
{
namespace cont
{
namespace testing
{

template <class DeviceAdapterTag>
struct TestingRuntimeDeviceConfiguration
{

  VTKM_CONT
  static internal::RuntimeDeviceConfigurationOptions DefaultInitializeConfigOptions()
  {
    internal::RuntimeDeviceConfigurationOptions runtimeDeviceOptions{};
    runtimeDeviceOptions.VTKmNumThreads.SetOption(8);
    runtimeDeviceOptions.VTKmNumaRegions.SetOption(0);
    runtimeDeviceOptions.VTKmDeviceInstance.SetOption(2);
    runtimeDeviceOptions.Initialize(nullptr);
    VTKM_TEST_ASSERT(runtimeDeviceOptions.IsInitialized(),
                     "Failed to default initialize runtime config options.");
    return runtimeDeviceOptions;
  }

  VTKM_CONT
  static void TestSerial() {}

  VTKM_CONT
  static void TestTBB() {}

  VTKM_CONT
  static void TestOpenMP() {}

  VTKM_CONT
  static void TestCuda() {}

  VTKM_CONT
  static void TestKokkos()
  {
    int argc;
    char** argv;
    vtkm::cont::testing::Testing::MakeArgs(argc, argv, "--kokkos-numa=4");
    vtkm::cont::testing::Testing::SetEnv("KOKKOS_DEVICE_ID", "0");
    auto deviceOptions = TestingRuntimeDeviceConfiguration::DefaultInitializeConfigOptions();
    bool threw = false;
    try
    {
      RuntimeDeviceInformation{}.GetRuntimeConfiguration(
        DeviceAdapterTag(), deviceOptions, argc, argv);
    }
    catch (const std::runtime_error& e)
    {
      threw = true;
    }
    VTKM_TEST_ASSERT(
      threw, "GetRuntimeConfiguration should have thrown, env KOKKOS_DEVICE_ID didn't match");
    VTKM_TEST_ASSERT(!Kokkos::is_initialized(), "Kokkos should not be initialized at this point");
    deviceOptions.VTKmDeviceInstance.SetOption(0);
    internal::RuntimeDeviceConfigurationBase& config =
      RuntimeDeviceInformation{}.GetRuntimeConfiguration(
        DeviceAdapterTag(), deviceOptions, argc, argv);
    VTKM_TEST_ASSERT(Kokkos::is_initialized(), "Kokkos should be initialized at this point");

    // Test that args are set and the right arg priority is applied
    vtkm::Id testValue;
    VTKM_TEST_ASSERT(config.GetThreads(testValue) ==
                       internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                     "Failed to get set threads");
    VTKM_TEST_ASSERT(testValue == 8,
                     "Set threads does not match expected value: 8 != " +
                       std::to_string(testValue));
    VTKM_TEST_ASSERT(config.GetNumaRegions(testValue) ==
                       internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                     "Failed to get set numa regions");
    VTKM_TEST_ASSERT(testValue == 4,
                     "Set numa regions does not match expected value: 4 != " +
                       std::to_string(testValue));
    VTKM_TEST_ASSERT(config.GetDeviceInstance(testValue) ==
                       internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                     "Failed to get set device instance");
    VTKM_TEST_ASSERT(testValue == 0,
                     "Set device instance does not match expected value: 0 != " +
                       std::to_string(testValue));

    // Ensure that with kokkos we can't re-initialize or set values after the first initialize
    // Should pop up a few warnings in the test logs
    deviceOptions.VTKmNumThreads.SetOption(16);
    deviceOptions.VTKmNumaRegions.SetOption(2);
    deviceOptions.VTKmDeviceInstance.SetOption(5);
    config.Initialize(deviceOptions);
    VTKM_TEST_ASSERT(config.SetThreads(1) == internal::RuntimeDeviceConfigReturnCode::NOT_APPLIED,
                     "Shouldn't be able to set threads after kokkos is initalized");
    VTKM_TEST_ASSERT(config.SetNumaRegions(1) ==
                       internal::RuntimeDeviceConfigReturnCode::NOT_APPLIED,
                     "Shouldn't be able to set numa regions after kokkos is initalized");
    VTKM_TEST_ASSERT(config.SetDeviceInstance(1) ==
                       internal::RuntimeDeviceConfigReturnCode::NOT_APPLIED,
                     "Shouldn't be able to set device instnace after kokkos is initalized");

    // make sure all the values are the same
    VTKM_TEST_ASSERT(config.GetThreads(testValue) ==
                       internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                     "Failed to get set threads");
    VTKM_TEST_ASSERT(testValue == 8,
                     "Set threads does not match expected value: 8 != " +
                       std::to_string(testValue));
    VTKM_TEST_ASSERT(config.GetNumaRegions(testValue) ==
                       internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                     "Failed to get set numa regions");
    VTKM_TEST_ASSERT(testValue == 4,
                     "Set numa regions does not match expected value: 4 != " +
                       std::to_string(testValue));
    VTKM_TEST_ASSERT(config.GetDeviceInstance(testValue) ==
                       internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                     "Failed to get set device instance");
    VTKM_TEST_ASSERT(testValue == 0,
                     "Set device instance does not match expected value: 0 != " +
                       std::to_string(testValue));

    vtkm::cont::testing::Testing::UnsetEnv("KOKKOS_DEVICE_ID");
  }

  struct TestRunner
  {
    VTKM_CONT
    void operator()() const
    {
      switch (DeviceAdapterTag{}.GetValue())
      {
        case vtkm::cont::DeviceAdapterTagSerial{}.GetValue():
          TestingRuntimeDeviceConfiguration::TestSerial();
          break;
        case vtkm::cont::DeviceAdapterTagTBB{}.GetValue():
          TestingRuntimeDeviceConfiguration::TestTBB();
          break;
        case vtkm::cont::DeviceAdapterTagOpenMP{}.GetValue():
          TestingRuntimeDeviceConfiguration::TestOpenMP();
          break;
        case vtkm::cont::DeviceAdapterTagCuda{}.GetValue():
          TestingRuntimeDeviceConfiguration::TestCuda();
          break;
        case vtkm::cont::DeviceAdapterTagKokkos{}.GetValue():
          TestingRuntimeDeviceConfiguration::TestKokkos();
          break;
        default:
          break;
      }
    }
  };

  static VTKM_CONT int Run(int, char*[])
  {
    // For the Kokkos version of this test we don't not want Initialize to be called
    // so we directly execute the testing functor instead of calling Run
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapterTag());
    return vtkm::cont::testing::Testing::ExecuteFunction(TestRunner{});
  }
};

} // namespace vtkm::cont::testing
} // namespace vtkm::cont
} // namespace vtkm

#endif // vtk_m_cont_testing_TestingRuntimeDeviceConfiguration_h
