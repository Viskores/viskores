//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/cont/testing/Testing.h>

#include <future>
#include <vector>

namespace
{

bool CheckLocalRuntime()
{
  if (!vtkm::cont::GetRuntimeDeviceTracker().CanRunOn(vtkm::cont::DeviceAdapterTagSerial{}))
  {
    std::cout << "Serial device not runable" << std::endl;
    return false;
  }

  for (vtkm::Int8 deviceIndex = 0; deviceIndex < VTKM_MAX_DEVICE_ADAPTER_ID; ++deviceIndex)
  {
    vtkm::cont::DeviceAdapterId device = vtkm::cont::make_DeviceAdapterId(deviceIndex);
    if (!device.IsValueValid() || (deviceIndex == VTKM_DEVICE_ADAPTER_SERIAL))
    {
      continue;
    }
    if (vtkm::cont::GetRuntimeDeviceTracker().CanRunOn(device))
    {
      std::cout << "Device " << device.GetName() << " declared as runnable" << std::endl;
      return false;
    }
  }

  return true;
}

void DoTest()
{
  VTKM_TEST_ASSERT(CheckLocalRuntime(),
                   "Runtime check failed on main thread. Did you try to set a device argument?");

  // Now check on a new thread. The runtime is a thread-local object so that each thread can
  // use its own device. But when you start a thread, you want the default to be what the
  // user selected on the main thread.
  VTKM_TEST_ASSERT(std::async(std::launch::async, CheckLocalRuntime).get(),
                   "Runtime loses defaults in spawned thread.");
}

} // anonymous namespace

int UnitTestDeviceSelectOnThreads(int argc, char* argv[])
{
  // This test is checking to make sure that a device selected in the command line
  // argument is the default for all threads. We will test this by adding an argument
  // to select the serial device, which is always available. The test might fail if
  // a different device is also selected.
  std::string deviceSelectString("--vtkm-device=serial");
  std::vector<char> deviceSelectArg(deviceSelectString.size());
  std::copy(deviceSelectString.begin(), deviceSelectString.end(), deviceSelectArg.begin());
  deviceSelectArg.push_back('\0');

  std::vector<char*> newArgs;
  for (int i = 0; i < argc; ++i)
  {
    if (std::strncmp(argv[i], "--vtkm-device", 13) != 0)
    {
      newArgs.push_back(argv[i]);
    }
  }
  newArgs.push_back(deviceSelectArg.data());
  newArgs.push_back(nullptr);

  int newArgc = static_cast<int>(newArgs.size() - 1);

  return vtkm::cont::testing::Testing::Run(DoTest, newArgc, newArgs.data());
}
