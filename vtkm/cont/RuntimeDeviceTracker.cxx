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
  RuntimeDeviceTrackerInternals() = default;

  RuntimeDeviceTrackerInternals(const RuntimeDeviceTrackerInternals* v) { this->CopyFrom(v); }

  RuntimeDeviceTrackerInternals& operator=(const RuntimeDeviceTrackerInternals* v)
  {
    this->CopyFrom(v);
    return *this;
  }

  bool GetRuntimeAllowed(std::size_t deviceId) const { return this->RuntimeAllowed[deviceId]; }
  void SetRuntimeAllowed(std::size_t deviceId, bool flag) { this->RuntimeAllowed[deviceId] = flag; }

  bool GetThreadFriendlyMemAlloc(std::size_t deviceId) const
  {
    return this->ThreadFriendlyMemAlloc[deviceId];
  }
  void SetThreadFriendlyMemAlloc(std::size_t deviceId, bool flag)
  {
    this->ThreadFriendlyMemAlloc[deviceId] = flag;
  }

  void ResetRuntimeAllowed()
  {
    std::fill_n(this->RuntimeAllowed, VTKM_MAX_DEVICE_ADAPTER_ID, false);
  }

  void ResetThreadFriendlyMemAlloc()
  {
    std::fill_n(this->ThreadFriendlyMemAlloc, VTKM_MAX_DEVICE_ADAPTER_ID, false);
  }

  void Reset()
  {
    this->ResetRuntimeAllowed();
    this->ResetThreadFriendlyMemAlloc();
  }

private:
  void CopyFrom(const RuntimeDeviceTrackerInternals* v)
  {
    std::copy(std::cbegin(v->RuntimeAllowed),
              std::cend(v->RuntimeAllowed),
              std::begin(this->RuntimeAllowed));
    std::copy(std::cbegin(v->ThreadFriendlyMemAlloc),
              std::cend(v->ThreadFriendlyMemAlloc),
              std::begin(this->ThreadFriendlyMemAlloc));
  }

  bool RuntimeAllowed[VTKM_MAX_DEVICE_ADAPTER_ID];
  bool ThreadFriendlyMemAlloc[VTKM_MAX_DEVICE_ADAPTER_ID];
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
RuntimeDeviceTracker::~RuntimeDeviceTracker() {}

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
      if (this->Internals->GetRuntimeAllowed(static_cast<std::size_t>(i)))
      {
        return true;
      }
    }
    return false;
  }
  else
  {
    this->CheckDevice(deviceId);
    return this->Internals->GetRuntimeAllowed(deviceId.GetValue());
  }
}

VTKM_CONT
bool RuntimeDeviceTracker::GetThreadFriendlyMemAlloc(vtkm::cont::DeviceAdapterId deviceId) const
{
  if (deviceId == vtkm::cont::DeviceAdapterTagAny{})
  { //If at least a single device is enabled, than any device is enabled
    for (vtkm::Int8 i = 1; i < VTKM_MAX_DEVICE_ADAPTER_ID; ++i)
    {
      if (this->Internals->GetThreadFriendlyMemAlloc(static_cast<std::size_t>(i)))
      {
        return true;
      }
    }
    return false;
  }
  else
  {
    this->CheckDevice(deviceId);
    return this->Internals->GetThreadFriendlyMemAlloc(deviceId.GetValue());
  }
}

VTKM_CONT
void RuntimeDeviceTracker::SetDeviceState(vtkm::cont::DeviceAdapterId deviceId, bool state)
{
  this->CheckDevice(deviceId);

  this->Internals->SetRuntimeAllowed(deviceId.GetValue(), state);
}

VTKM_CONT
void RuntimeDeviceTracker::SetThreadFriendlyMemAlloc(vtkm::cont::DeviceAdapterId deviceId,
                                                     bool state)
{
  this->CheckDevice(deviceId);

  this->Internals->SetThreadFriendlyMemAlloc(deviceId.GetValue(), state);
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
  this->Internals->Reset();

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
      this->Internals->SetRuntimeAllowed(device.GetValue(), state);
    }
  }
  this->LogEnabledDevices();
}

VTKM_CONT void RuntimeDeviceTracker::DisableDevice(vtkm::cont::DeviceAdapterId deviceId)
{
  if (deviceId == vtkm::cont::DeviceAdapterTagAny{})
  {
    this->Internals->ResetRuntimeAllowed();
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

    this->Internals->ResetRuntimeAllowed();
    this->Internals->SetRuntimeAllowed(deviceId.GetValue(), runtimeExists);
    this->LogEnabledDevices();
  }
}

VTKM_CONT void RuntimeDeviceTracker::CopyStateFrom(const vtkm::cont::RuntimeDeviceTracker& tracker)
{
  *(this->Internals) = tracker.Internals;
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
  , SavedState(new detail::RuntimeDeviceTrackerInternals(this->Internals))
{
  VTKM_LOG_S(vtkm::cont::LogLevel::DevicesEnabled, "Entering scoped runtime region");

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
  , SavedState(new detail::RuntimeDeviceTrackerInternals(this->Internals))
{
  VTKM_LOG_S(vtkm::cont::LogLevel::DevicesEnabled, "Entering scoped runtime region");

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
  , SavedState(new detail::RuntimeDeviceTrackerInternals(this->Internals))
{
  VTKM_LOG_S(vtkm::cont::LogLevel::DevicesEnabled, "Entering scoped runtime region");
}

VTKM_CONT
ScopedRuntimeDeviceTracker::~ScopedRuntimeDeviceTracker()
{
  VTKM_LOG_S(vtkm::cont::LogLevel::DevicesEnabled, "Leaving scoped runtime region");
  *(this->Internals) = this->SavedState.get();

  this->LogEnabledDevices();
}

VTKM_CONT
vtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker()
{
  using SharedTracker = std::shared_ptr<vtkm::cont::RuntimeDeviceTracker>;
  static thread_local vtkm::cont::detail::RuntimeDeviceTrackerInternals details;
  static thread_local SharedTracker runtimeDeviceTracker;
  static std::weak_ptr<vtkm::cont::RuntimeDeviceTracker> defaultRuntimeDeviceTracker;

  if (runtimeDeviceTracker)
  {
    return *runtimeDeviceTracker;
  }

  // The RuntimeDeviceTracker for this thread has not been created. Create a new one.
  runtimeDeviceTracker = SharedTracker(new vtkm::cont::RuntimeDeviceTracker(&details, true));

  // Get the default details, which are a global variable, with thread safety
  static std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);

  SharedTracker defaultTracker = defaultRuntimeDeviceTracker.lock();

  if (defaultTracker)
  {
    // We already have a default tracker, so copy the state from there. We don't need to
    // keep our mutex locked because we already have a safe handle to the defaultTracker.
    lock.unlock();
    runtimeDeviceTracker->CopyStateFrom(*defaultTracker);
  }
  else
  {
    // There is no default tracker yet. It has never been created (or it was on a thread
    // that got deleted). Use the current thread's details as the default.
    defaultRuntimeDeviceTracker = runtimeDeviceTracker;
  }

  return *runtimeDeviceTracker;
}

}
} // namespace vtkm::cont
