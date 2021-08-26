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
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
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
TestingRuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagCuda>::TestRuntimeConfig()
{
  auto deviceOptions = TestingRuntimeDeviceConfiguration::DefaultInitializeConfigOptions();
  int numDevices = 0;
  VTKM_CUDA_CALL(cudaGetDeviceCount(&numDevices));
  vtkm::Id selectedDevice = numDevices > 0 ? numDevices - 1 : 0;
  deviceOptions.VTKmDeviceInstance.SetOption(selectedDevice);
  auto& config =
    RuntimeDeviceInformation{}.GetRuntimeConfiguration(DeviceAdapterTagCuda(), deviceOptions);
  vtkm::Id setDevice;
  VTKM_TEST_ASSERT(config.GetDeviceInstance(setDevice) ==
                     internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                   "Failed to get device instance");
  VTKM_TEST_ASSERT(setDevice == selectedDevice,
                   "RTC's setDevice != selectedDevice cuda direct! " + std::to_string(setDevice) +
                     " != " + std::to_string(selectedDevice));
  vtkm::Id maxDevices;
  VTKM_TEST_ASSERT(config.GetMaxDevices(maxDevices) ==
                     internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                   "Failed to get max devices");
  VTKM_TEST_ASSERT(maxDevices == numDevices,
                   "RTC's maxDevices != numDevices cuda direct! " + std::to_string(maxDevices) +
                     " != " + std::to_string(numDevices));
  std::vector<cudaDeviceProp> cudaProps;
  dynamic_cast<internal::RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagCuda>&>(config)
    .GetCudaDeviceProp(cudaProps);
  VTKM_TEST_ASSERT(maxDevices == static_cast<vtkm::Id>(cudaProps.size()),
                   "CudaProp's size != maxDevices! " + std::to_string(cudaProps.size()) +
                     " != " + std::to_string(maxDevices));
}

} // namespace vtkm::cont::testing
} // namespace vtkm::cont
} // namespace vtkm

int UnitTestCudaRuntimeDeviceConfiguration(int argc, char* argv[])
{
  return vtkm::cont::testing::TestingRuntimeDeviceConfiguration<
    vtkm::cont::DeviceAdapterTagCuda>::Run(argc, argv);
}
