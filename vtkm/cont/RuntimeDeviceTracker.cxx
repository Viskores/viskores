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

#include <vtkm/cont/ErrorBadValue.h>

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
RuntimeDeviceTracker::RuntimeDeviceTracker(detail::RuntimeDeviceTrackerInternals* details,
                                           bool reset)
  : Internals(details)
{
  if (reset)
  {
    this->Reset();
  }
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
bool RuntimeDeviceTracker::CanRunOn(vtkm::cont::DeviceAdapterId deviceId) const
{
  if (deviceId == vtkm::cont::DeviceAdapterTagAny{})
  { //If at least a single device is enabled, than any device is enabled
    for (vtkm::Int8 i = 1; i < VTKM_MAX_DEVICE_ADAPTER_ID; ++i)
    {
      if (this->Internals->RuntimeAllowed[static_cast<std::size_t>(i)])
      {
        return true;
      }
    }
    return false;
  }
  else
  {
    this->CheckDevice(deviceId);
    return this->Internals->RuntimeAllowed[deviceId.GetValue()];
  }
}

VTKM_CONT
void RuntimeDeviceTracker::SetDeviceState(vtkm::cont::DeviceAdapterId deviceId, bool state)
{
  this->CheckDevice(deviceId);

  this->Internals->RuntimeAllowed[deviceId.GetValue()] = state;
}


VTKM_CONT void RuntimeDeviceTracker::ResetDevice(vtkm::cont::DeviceAdapterId deviceId)
{
  if (deviceId == vtkm::cont::DeviceAdapterTagAny{})
  {
    this->Reset();
  }
  else
  {
    vtkm::cont::RuntimeDeviceInformation runtimeDevice;
    this->SetDeviceState(deviceId, runtimeDevice.Exists(deviceId));
    this->LogEnabledDevices();
  }
}


VTKM_CONT
void RuntimeDeviceTracker::Reset()
{
  std::fill_n(this->Internals->RuntimeAllowed, VTKM_MAX_DEVICE_ADAPTER_ID, false);

  // We use this instead of calling CheckDevice/SetDeviceState so that
  // when we use logging we get better messages stating we are reseting
  // the devices.
  vtkm::cont::RuntimeDeviceInformation runtimeDevice;
  for (vtkm::Int8 i = 1; i < VTKM_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    vtkm::cont::DeviceAdapterId device = vtkm::cont::make_DeviceAdapterId(i);
    if (device.IsValueValid())
    {
      const bool state = runtimeDevice.Exists(device);
      this->Internals->RuntimeAllowed[device.GetValue()] = state;
    }
  }
  this->LogEnabledDevices();
}

VTKM_CONT void RuntimeDeviceTracker::DisableDevice(vtkm::cont::DeviceAdapterId deviceId)
{
  if (deviceId == vtkm::cont::DeviceAdapterTagAny{})
  {
    std::fill_n(this->Internals->RuntimeAllowed, VTKM_MAX_DEVICE_ADAPTER_ID, false);
  }
  else
  {
    this->SetDeviceState(deviceId, false);
  }
  this->LogEnabledDevices();
}

VTKM_CONT
void RuntimeDeviceTracker::ForceDevice(DeviceAdapterId deviceId)
{
  if (deviceId == vtkm::cont::DeviceAdapterTagAny{})
  {
    this->Reset();
  }
  else
  {
    this->CheckDevice(deviceId);
    vtkm::cont::RuntimeDeviceInformation runtimeDevice;
    const bool runtimeExists = runtimeDevice.Exists(deviceId);
    if (!runtimeExists)
    {
      std::stringstream message;
      message << "Cannot force to device '" << deviceId.GetName()
              << "' because that device is not available on this system";
      throw vtkm::cont::ErrorBadValue(message.str());
    }

    std::fill_n(this->Internals->RuntimeAllowed, VTKM_MAX_DEVICE_ADAPTER_ID, false);

    this->Internals->RuntimeAllowed[deviceId.GetValue()] = runtimeExists;
    this->LogEnabledDevices();
  }
}

VTKM_CONT
void RuntimeDeviceTracker::PrintSummary(std::ostream& out) const
{
  for (vtkm::Int8 i = 1; i < VTKM_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    auto dev = vtkm::cont::make_DeviceAdapterId(i);
    out << " - Device " << static_cast<vtkm::Int32>(i) << " (" << dev.GetName()
        << "): Enabled=" << this->CanRunOn(dev) << "\n";
  }
}

VTKM_CONT
void RuntimeDeviceTracker::LogEnabledDevices() const
{
  std::stringstream message;
  message << "Enabled devices:";
  bool atLeastOneDeviceEnabled = false;
  for (vtkm::Int8 deviceIndex = 1; deviceIndex < VTKM_MAX_DEVICE_ADAPTER_ID; ++deviceIndex)
  {
    vtkm::cont::DeviceAdapterId device = vtkm::cont::make_DeviceAdapterId(deviceIndex);
    if (this->CanRunOn(device))
    {
      message << " " << device.GetName();
      atLeastOneDeviceEnabled = true;
    }
  }
  if (!atLeastOneDeviceEnabled)
  {
    message << " NONE!";
  }
  VTKM_LOG_S(vtkm::cont::LogLevel::DevicesEnabled, message.str());
}

VTKM_CONT
ScopedRuntimeDeviceTracker::ScopedRuntimeDeviceTracker(vtkm::cont::DeviceAdapterId device,
                                                       RuntimeDeviceTrackerMode mode)
  : RuntimeDeviceTracker(GetRuntimeDeviceTracker().Internals, false)
  , SavedState(new detail::RuntimeDeviceTrackerInternals())
{
  VTKM_LOG_S(vtkm::cont::LogLevel::DevicesEnabled, "Entering scoped runtime region");
  std::copy_n(
    this->Internals->RuntimeAllowed, VTKM_MAX_DEVICE_ADAPTER_ID, this->SavedState->RuntimeAllowed);

  if (mode == RuntimeDeviceTrackerMode::Force)
  {
    this->ForceDevice(device);
  }
  else if (mode == RuntimeDeviceTrackerMode::Enable)
  {
    this->ResetDevice(device);
  }
  else if (mode == RuntimeDeviceTrackerMode::Disable)
  {
    this->DisableDevice(device);
  }
}

VTKM_CONT
ScopedRuntimeDeviceTracker::ScopedRuntimeDeviceTracker(
  vtkm::cont::DeviceAdapterId device,
  RuntimeDeviceTrackerMode mode,
  const vtkm::cont::RuntimeDeviceTracker& tracker)
  : RuntimeDeviceTracker(tracker.Internals, false)
  , SavedState(new detail::RuntimeDeviceTrackerInternals())
{
  VTKM_LOG_S(vtkm::cont::LogLevel::DevicesEnabled, "Entering scoped runtime region");
  std::copy_n(
    this->Internals->RuntimeAllowed, VTKM_MAX_DEVICE_ADAPTER_ID, this->SavedState->RuntimeAllowed);
  if (mode == RuntimeDeviceTrackerMode::Force)
  {
    this->ForceDevice(device);
  }
  else if (mode == RuntimeDeviceTrackerMode::Enable)
  {
    this->ResetDevice(device);
  }
  else if (mode == RuntimeDeviceTrackerMode::Disable)
  {
    this->DisableDevice(device);
  }
}

VTKM_CONT
ScopedRuntimeDeviceTracker::ScopedRuntimeDeviceTracker(
  const vtkm::cont::RuntimeDeviceTracker& tracker)
  : RuntimeDeviceTracker(tracker.Internals, false)
  , SavedState(new detail::RuntimeDeviceTrackerInternals())
{
  VTKM_LOG_S(vtkm::cont::LogLevel::DevicesEnabled, "Entering scoped runtime region");
  std::copy_n(
    this->Internals->RuntimeAllowed, VTKM_MAX_DEVICE_ADAPTER_ID, this->SavedState->RuntimeAllowed);
}

VTKM_CONT
ScopedRuntimeDeviceTracker::~ScopedRuntimeDeviceTracker()
{
  VTKM_LOG_S(vtkm::cont::LogLevel::DevicesEnabled, "Leaving scoped runtime region");
  std::copy_n(
    this->SavedState->RuntimeAllowed, VTKM_MAX_DEVICE_ADAPTER_ID, this->Internals->RuntimeAllowed);
  this->LogEnabledDevices();
}

VTKM_CONT
vtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker()
{
#if defined(VTKM_CLANG) && defined(__apple_build_version__) && (__apple_build_version__ < 8000000)
  static std::mutex mtx;
  static std::map<std::thread::id, vtkm::cont::RuntimeDeviceTracker*> globalTrackers;
  static std::map<std::thread::id, vtkm::cont::detail::RuntimeDeviceTrackerInternals*>
    globalTrackerInternals;
  std::thread::id this_id = std::this_thread::get_id();

  std::unique_lock<std::mutex> lock(mtx);
  auto iter = globalTrackers.find(this_id);
  if (iter != globalTrackers.end())
  {
    return *iter->second;
  }
  else
  {
    auto* details = new vtkm::cont::detail::RuntimeDeviceTrackerInternals();
    vtkm::cont::RuntimeDeviceTracker* tracker = new vtkm::cont::RuntimeDeviceTracker(details, true);
    globalTrackers[this_id] = tracker;
    globalTrackerInternals[this_id] = details;
    return *tracker;
  }
#else
  static thread_local vtkm::cont::detail::RuntimeDeviceTrackerInternals details;
  static thread_local vtkm::cont::RuntimeDeviceTracker runtimeDeviceTracker(&details, true);
  return runtimeDeviceTracker;
#endif
}
}
} // namespace vtkm::cont
