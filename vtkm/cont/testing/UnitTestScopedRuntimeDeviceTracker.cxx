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

//include all backends
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/openmp/DeviceAdapterOpenMP.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <array>
#include <thread>

namespace
{

void verify_state(vtkm::cont::DeviceAdapterId tag,
                  std::array<bool, VTKM_MAX_DEVICE_ADAPTER_ID>& defaults)
{
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  // presumable all other devices match the defaults
  for (vtkm::Int8 i = 1; i < VTKM_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    const auto deviceId = vtkm::cont::make_DeviceAdapterId(i);
    if (deviceId != tag)
    {
      VTKM_TEST_ASSERT(defaults[static_cast<std::size_t>(i)] == tracker.CanRunOn(deviceId),
                       "ScopedRuntimeDeviceTracker didn't properly setup state correctly");
    }
  }
}

void verify_state_thread(vtkm::cont::DeviceAdapterId tag,
                         std::array<bool, VTKM_MAX_DEVICE_ADAPTER_ID>& defaults,
                         const vtkm::cont::RuntimeDeviceTracker& tracker)
{
  // Each thread has its own RuntimeDeviceTracker (to allow you to control different devices
  // on different threads). But that means that each thread creates its own tracker. We
  // want all the threads to respect the runtime set up on the main thread, so copy the state
  // of that tracker (passed as an argument) to this thread.
  vtkm::cont::GetRuntimeDeviceTracker().CopyStateFrom(tracker);

  verify_state(tag, defaults);
}

void verify_srdt_support(vtkm::cont::DeviceAdapterId tag,
                         std::array<bool, VTKM_MAX_DEVICE_ADAPTER_ID>& force,
                         std::array<bool, VTKM_MAX_DEVICE_ADAPTER_ID>& enable,
                         std::array<bool, VTKM_MAX_DEVICE_ADAPTER_ID>& disable)
{
  vtkm::cont::RuntimeDeviceInformation runtime;
  const bool haveSupport = runtime.Exists(tag);
  if (haveSupport)
  {
    vtkm::cont::ScopedRuntimeDeviceTracker tracker(tag,
                                                   vtkm::cont::RuntimeDeviceTrackerMode::Force);
    VTKM_TEST_ASSERT(tracker.CanRunOn(tag) == haveSupport, "");
    verify_state(tag, force);
    std::thread(verify_state_thread, tag, std::ref(force), std::ref(tracker)).join();
  }

  if (haveSupport)
  {
    vtkm::cont::ScopedRuntimeDeviceTracker tracker(tag,
                                                   vtkm::cont::RuntimeDeviceTrackerMode::Enable);
    VTKM_TEST_ASSERT(tracker.CanRunOn(tag) == haveSupport);
    verify_state(tag, enable);
    std::thread(verify_state_thread, tag, std::ref(enable), std::ref(tracker)).join();
  }

  {
    vtkm::cont::ScopedRuntimeDeviceTracker tracker(tag,
                                                   vtkm::cont::RuntimeDeviceTrackerMode::Disable);
    VTKM_TEST_ASSERT(tracker.CanRunOn(tag) == false, "");
    verify_state(tag, disable);
    std::thread(verify_state_thread, tag, std::ref(disable), std::ref(tracker)).join();
  }
}

void VerifyScopedRuntimeDeviceTracker()
{
  std::array<bool, VTKM_MAX_DEVICE_ADAPTER_ID> all_off;
  std::array<bool, VTKM_MAX_DEVICE_ADAPTER_ID> all_on;
  std::array<bool, VTKM_MAX_DEVICE_ADAPTER_ID> defaults;

  all_off.fill(false);
  vtkm::cont::RuntimeDeviceInformation runtime;
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  for (vtkm::Int8 i = 1; i < VTKM_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    auto deviceId = vtkm::cont::make_DeviceAdapterId(i);
    defaults[static_cast<std::size_t>(i)] = tracker.CanRunOn(deviceId);
    all_on[static_cast<std::size_t>(i)] = runtime.Exists(deviceId);
  }

  using SerialTag = ::vtkm::cont::DeviceAdapterTagSerial;
  using OpenMPTag = ::vtkm::cont::DeviceAdapterTagOpenMP;
  using TBBTag = ::vtkm::cont::DeviceAdapterTagTBB;
  using CudaTag = ::vtkm::cont::DeviceAdapterTagCuda;
  using KokkosTag = ::vtkm::cont::DeviceAdapterTagKokkos;
  using AnyTag = ::vtkm::cont::DeviceAdapterTagAny;

  //Verify that for each device adapter we compile code for, that it
  //has valid runtime support.
  verify_srdt_support(SerialTag(), all_off, all_on, defaults);
  verify_srdt_support(OpenMPTag(), all_off, all_on, defaults);
  verify_srdt_support(CudaTag(), all_off, all_on, defaults);
  verify_srdt_support(TBBTag(), all_off, all_on, defaults);
  verify_srdt_support(KokkosTag(), all_off, all_on, defaults);

  // Verify that all the ScopedRuntimeDeviceTracker changes
  // have been reverted
  verify_state(AnyTag(), defaults);


  verify_srdt_support(AnyTag(), all_on, all_on, all_off);

  // Verify that all the ScopedRuntimeDeviceTracker changes
  // have been reverted
  verify_state(AnyTag(), defaults);
}

} // anonymous namespace

int UnitTestScopedRuntimeDeviceTracker(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(VerifyScopedRuntimeDeviceTracker, argc, argv);
}
