//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DeviceAdapterListTag.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/cont/testing/Testing.h>

#include <chrono>
#include <thread>

namespace
{

struct TimerTestDevices
  : vtkm::ListTagAppend<VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG, vtkm::cont::DeviceAdapterTagAny>
{
};

constexpr long long waitTimeMilliseconds = 250;
constexpr vtkm::Float64 waitTimeSeconds = vtkm::Float64(waitTimeMilliseconds) / 1000;

struct Waiter
{
  std::chrono::high_resolution_clock::time_point Start = std::chrono::high_resolution_clock::now();
  long long ExpectedTimeMilliseconds = 0;

  vtkm::Float64 Wait()
  {
    // Update when we want to wait to.
    this->ExpectedTimeMilliseconds += waitTimeMilliseconds;
    vtkm::Float64 expectedTimeSeconds = vtkm::Float64(this->ExpectedTimeMilliseconds) / 1000;

    long long elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::high_resolution_clock::now() - this->Start)
                                      .count();

    long long millisecondsToSleep = this->ExpectedTimeMilliseconds - elapsedMilliseconds;

    std::cout << "  Sleeping for " << millisecondsToSleep << "ms (to " << expectedTimeSeconds
              << "s)" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsToSleep));

    VTKM_TEST_ASSERT(std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::high_resolution_clock::now() - this->Start)
                         .count() <
                       (this->ExpectedTimeMilliseconds + ((3 * waitTimeMilliseconds) / 4)),
                     "Internal test error: Sleep lasted longer than expected. System must be busy. "
                     "Might need to increase waitTimeMilliseconds.");

    return expectedTimeSeconds;
  }
};

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
    return vtkm::cont::GetRuntimeDeviceTracker().CanRunOn(device);
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
      VTKM_TEST_ASSERT(elapsedTime < (expectedTime + waitTimeSeconds),
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
  std::cout << "  Check time for " << expectedTime << "s" << std::endl;
  vtkm::ListForEach(CheckTimeForDeviceFunctor(), TimerTestDevices(), timer, expectedTime);
}

void DoTimerCheck(vtkm::cont::Timer& timer)
{
  std::cout << "  Starting timer" << std::endl;
  timer.Start();
  VTKM_TEST_ASSERT(timer.Started(), "Timer fails to track started status");
  VTKM_TEST_ASSERT(!timer.Stopped(), "Timer fails to track non stopped status");

  Waiter waiter;

  vtkm::Float64 expectedTime = 0.0;
  CheckTime(timer, expectedTime);

  expectedTime = waiter.Wait();

  CheckTime(timer, expectedTime);

  std::cout << "  Make sure timer is still running" << std::endl;
  VTKM_TEST_ASSERT(!timer.Stopped(), "Timer fails to track stopped status");

  expectedTime = waiter.Wait();

  CheckTime(timer, expectedTime);

  std::cout << "  Stop the timer" << std::endl;
  timer.Stop();
  VTKM_TEST_ASSERT(timer.Stopped(), "Timer fails to track stopped status");

  CheckTime(timer, expectedTime);

  waiter.Wait(); // Do not advanced expected time

  std::cout << "  Check that timer legitimately stopped" << std::endl;
  CheckTime(timer, expectedTime);
}

struct TimerCheckFunctor
{
  void operator()(vtkm::cont::DeviceAdapterId device) const
  {
    if ((device != vtkm::cont::DeviceAdapterTagAny()) &&
        !vtkm::cont::GetRuntimeDeviceTracker().CanRunOn(device))
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
