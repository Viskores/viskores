//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/DeviceAdapterListTag.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/cont/internal/DeviceAdapterError.h>

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/internal/Windows.h>
namespace
{

struct TimerTestDevices
  : vtkm::ListTagAppend<VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG, vtkm::cont::DeviceAdapterTagAny>
{
};

void WaitASec()
{
  std::cout << "  Sleeping for 1 second" << std::endl;
#ifdef VTKM_WINDOWS
  Sleep(1000);
#else
  sleep(1);
#endif
}

bool CanTimeOnDevice(const vtkm::cont::Timer& timer, vtkm::cont::DeviceAdapterId device)
{
  if (device == vtkm::cont::DeviceAdapterTagAny())
  {
    // The timer can run on any device. It should pick up something (unless perhaps there are no
    // devices, which would only happen if you explicitly disable serial, which we don't).
    return true;
  }
  else if ((timer.GetDevice() == vtkm::cont::DeviceAdapterTagAny()) ||
           (timer.GetDevice() == device))
  {
    // Device is specified and it is a match for the timer's device.
    return vtkm::cont::GetGlobalRuntimeDeviceTracker().CanRunOn(device);
  }
  else
  {
    // The requested device does not match the device of the timer.
    return false;
  }
}

struct CheckTimeForDeviceFunctor
{
  void operator()(vtkm::cont::DeviceAdapterId device,
                  const vtkm::cont::Timer& timer,
                  vtkm::Float64 expectedTime) const
  {
    std::cout << "    Checking time for device " << device.GetName() << std::endl;
    if (CanTimeOnDevice(timer, device))
    {
      vtkm::Float64 elapsedTime = timer.GetElapsedTime(device);
      VTKM_TEST_ASSERT(
        elapsedTime > (expectedTime - 0.001), "Timer did not capture full wait. ", elapsedTime);
      VTKM_TEST_ASSERT(elapsedTime < (expectedTime + 1.0),
                       "Timer counted too far or system really busy. ",
                       elapsedTime);
    }
    else
    {
      std::cout << "      Device not supported. Expect 0 back and possible error in log."
                << std::endl;
      VTKM_TEST_ASSERT(timer.GetElapsedTime(device) == 0.0,
                       "Disabled timer should return nothing.");
    }
  }
};

void CheckTime(const vtkm::cont::Timer& timer, vtkm::Float64 expectedTime)
{
  vtkm::ListForEach(CheckTimeForDeviceFunctor(), TimerTestDevices(), timer, expectedTime);
}

void DoTimerCheck(vtkm::cont::Timer& timer)
{
  std::cout << "  Starting timer" << std::endl;
  timer.Start();
  VTKM_TEST_ASSERT(timer.Started(), "Timer fails to track started status");
  VTKM_TEST_ASSERT(!timer.Stopped(), "Timer fails to track non stopped status");

  WaitASec();

  std::cout << "  Check time for 1sec" << std::endl;
  CheckTime(timer, 1.0);

  std::cout << "  Make sure timer is still running" << std::endl;
  VTKM_TEST_ASSERT(!timer.Stopped(), "Timer fails to track stopped status");

  WaitASec();

  std::cout << "  Check time for 2sec" << std::endl;
  CheckTime(timer, 2.0);

  std::cout << "  Stop the timer" << std::endl;
  timer.Stop();
  VTKM_TEST_ASSERT(timer.Stopped(), "Timer fails to track stopped status");

  std::cout << "  Check time for 2sec" << std::endl;
  CheckTime(timer, 2.0);

  WaitASec();

  std::cout << "  Check that timer legitimately stopped at 2sec" << std::endl;
  CheckTime(timer, 2.0);
}

struct TimerCheckFunctor
{
  void operator()(vtkm::cont::DeviceAdapterId device) const
  {
    if ((device != vtkm::cont::DeviceAdapterTagAny()) &&
        !vtkm::cont::GetGlobalRuntimeDeviceTracker().CanRunOn(device))
    {
      // A timer will not work if set on a device that is not supported. Just skip this test.
      return;
    }

    {
      std::cout << "Checking Timer on device " << device.GetName() << " set with constructor"
                << std::endl;
      vtkm::cont::Timer timer(device);
      DoTimerCheck(timer);
    }
    {
      std::cout << "Checking Timer on device " << device.GetName() << " reset" << std::endl;
      vtkm::cont::Timer timer;
      timer.Reset(device);
      DoTimerCheck(timer);
    }
  }
};

void DoTimerTest()
{
  std::cout << "Check default timer" << std::endl;
  vtkm::cont::Timer timer;
  DoTimerCheck(timer);

  vtkm::ListForEach(TimerCheckFunctor(), TimerTestDevices());
}

} // anonymous namespace

int UnitTestTimer(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTimerTest, argc, argv);
}
