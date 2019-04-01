//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/internal/DeviceAdapterError.h>

#include <algorithm>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

namespace vtkm
{
namespace cont
{

namespace detail
{

struct RuntimeDeviceTrackerInternals
{
  bool RuntimeAllowed[VTKM_MAX_DEVICE_ADAPTER_ID];
};
}

VTKM_CONT
RuntimeDeviceTracker::RuntimeDeviceTracker()
  : Internals(std::make_shared<detail::RuntimeDeviceTrackerInternals>())
{
  this->Reset();
}

VTKM_CONT
RuntimeDeviceTracker::~RuntimeDeviceTracker()
{
}

VTKM_CONT
void RuntimeDeviceTracker::CheckDevice(vtkm::cont::DeviceAdapterId deviceId) const
{
  if (!deviceId.IsValueValid())
  {
    std::stringstream message;
    message << "Device '" << deviceId.GetName() << "' has invalid ID of "
            << (int)deviceId.GetValue();
    throw vtkm::cont::ErrorBadValue(message.str());
  }
}

VTKM_CONT
bool RuntimeDeviceTracker::CanRunOnImpl(vtkm::cont::DeviceAdapterId deviceId) const
{
  this->CheckDevice(deviceId);
  return this->Internals->RuntimeAllowed[deviceId.GetValue()];
}

VTKM_CONT
void RuntimeDeviceTracker::SetDeviceState(vtkm::cont::DeviceAdapterId deviceId, bool state)
{
  this->CheckDevice(deviceId);

  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             "Setting device '" << deviceId.GetName() << "' to " << state);
  this->Internals->RuntimeAllowed[deviceId.GetValue()] = state;
}

VTKM_CONT
void RuntimeDeviceTracker::Reset()
{
  std::fill_n(this->Internals->RuntimeAllowed, VTKM_MAX_DEVICE_ADAPTER_ID, false);

  // We use this instead of calling CheckDevice/SetDeviceState so that
  // when we use logging we get better messages stating we are reseting
  // the devices.
  vtkm::cont::RuntimeDeviceInformation runtimeDevice;
  for (vtkm::Int8 i = 0; i < VTKM_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    vtkm::cont::DeviceAdapterId device = vtkm::cont::make_DeviceAdapterId(i);
    if (device.IsValueValid())
    {
      const bool state = runtimeDevice.Exists(device);
      this->Internals->RuntimeAllowed[device.GetValue()] = state;
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 "Reset device '" << device.GetName() << "' to " << state);
    }
  }
}

VTKM_CONT
vtkm::cont::RuntimeDeviceTracker RuntimeDeviceTracker::DeepCopy() const
{
  return vtkm::cont::RuntimeDeviceTracker(this->Internals);
}

VTKM_CONT
void RuntimeDeviceTracker::DeepCopy(const vtkm::cont::RuntimeDeviceTracker& src)
{
  std::copy_n(
    src.Internals->RuntimeAllowed, VTKM_MAX_DEVICE_ADAPTER_ID, this->Internals->RuntimeAllowed);
}

VTKM_CONT
RuntimeDeviceTracker::RuntimeDeviceTracker(
  const std::shared_ptr<detail::RuntimeDeviceTrackerInternals>& internals)
  : Internals(std::make_shared<detail::RuntimeDeviceTrackerInternals>())
{
  std::copy_n(
    internals->RuntimeAllowed, VTKM_MAX_DEVICE_ADAPTER_ID, this->Internals->RuntimeAllowed);
}

VTKM_CONT
void RuntimeDeviceTracker::ForceDeviceImpl(vtkm::cont::DeviceAdapterId deviceId, bool runtimeExists)
{
  if (!runtimeExists)
  {
    std::stringstream message;
    message << "Cannot force to device '" << deviceId.GetName()
            << "' because that device is not available on this system";
    throw vtkm::cont::ErrorBadValue(message.str());
  }
  this->CheckDevice(deviceId);

  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             "Forcing execution to occur on device '" << deviceId.GetName() << "'");

  std::fill_n(this->Internals->RuntimeAllowed, VTKM_MAX_DEVICE_ADAPTER_ID, false);

  this->Internals->RuntimeAllowed[deviceId.GetValue()] = runtimeExists;
}

VTKM_CONT
void RuntimeDeviceTracker::ForceDevice(DeviceAdapterId deviceId)
{
  vtkm::cont::RuntimeDeviceInformation runtimeDevice;
  this->ForceDeviceImpl(deviceId, runtimeDevice.Exists(deviceId));
}

VTKM_CONT
vtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker()
{
#if defined(VTKM_CLANG) && defined(__apple_build_version__) && (__apple_build_version__ < 8000000)
  static std::mutex mtx;
  static std::map<std::thread::id, vtkm::cont::RuntimeDeviceTracker*> globalTrackers;
  std::thread::id this_id = std::this_thread::get_id();

  std::unique_lock<std::mutex> lock(mtx);
  auto iter = globalTrackers.find(this_id);
  if (iter != globalTrackers.end())
  {
    return *iter->second;
  }
  else
  {
    vtkm::cont::RuntimeDeviceTracker* tracker = new vtkm::cont::RuntimeDeviceTracker();
    globalTrackers[this_id] = tracker;
    return *tracker;
  }
#else
  static thread_local vtkm::cont::RuntimeDeviceTracker runtimeDeviceTracker;
  return runtimeDeviceTracker;
#endif
}
}
} // namespace vtkm::cont
