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

  VTKM_CONT static void TestRuntimeConfig(){};

  struct TestRunner
  {
    VTKM_CONT
    void operator()() const { TestRuntimeConfig(); }
  };

  static VTKM_CONT int Run(int, char*[])
  {
    // For the Kokkos version of this test we don't not want Initialize to be called
    // so we directly execute the testing functor instead of calling Run
    return vtkm::cont::testing::Testing::ExecuteFunction(TestRunner{});
  }
};

} // namespace vtkm::cont::testing
} // namespace vtkm::cont
} // namespace vtkm

#endif // vtk_m_cont_testing_TestingRuntimeDeviceConfiguration_h
