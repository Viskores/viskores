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

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterListTag.h>
#include <vtkm/cont/ErrorBadValue.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include <algorithm>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

namespace
{

struct VTKM_NEVER_EXPORT GetDeviceNameFunctor
{
  vtkm::cont::DeviceAdapterNameType* Names;

  VTKM_CONT
  GetDeviceNameFunctor(vtkm::cont::DeviceAdapterNameType* names)
    : Names(names)
  {
    std::fill_n(this->Names, VTKM_MAX_DEVICE_ADAPTER_ID, "InvalidDeviceId");
  }

  template <typename Device>
  VTKM_CONT void operator()(Device device)
  {
    auto id = device.GetValue();

    if (id > 0 && id < VTKM_MAX_DEVICE_ADAPTER_ID)
    {
      this->Names[id] = vtkm::cont::DeviceAdapterTraits<Device>::GetName();
    }
  }
};

} // end anon namespace

namespace vtkm
{
namespace cont
{

namespace detail
{

struct RuntimeDeviceTrackerInternals
{
  bool RuntimeValid[VTKM_MAX_DEVICE_ADAPTER_ID];
  DeviceAdapterNameType DeviceNames[VTKM_MAX_DEVICE_ADAPTER_ID];
};
}

VTKM_CONT
RuntimeDeviceTracker::RuntimeDeviceTracker()
  : Internals(new detail::RuntimeDeviceTrackerInternals)
{
  GetDeviceNameFunctor functor(this->Internals->DeviceNames);
  vtkm::ListForEach(functor, VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG());

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
  return this->Internals->RuntimeValid[deviceId.GetValue()];
}

VTKM_CONT
void RuntimeDeviceTracker::SetDeviceState(vtkm::cont::DeviceAdapterId deviceId, bool state)
{
  this->CheckDevice(deviceId);
  this->Internals->RuntimeValid[deviceId.GetValue()] = state;
}

namespace
{

struct VTKM_NEVER_EXPORT RuntimeDeviceTrackerResetFunctor
{
  vtkm::cont::RuntimeDeviceTracker Tracker;

  VTKM_CONT
  RuntimeDeviceTrackerResetFunctor(const vtkm::cont::RuntimeDeviceTracker& tracker)
    : Tracker(tracker)
  {
  }

  template <typename Device>
  VTKM_CONT void operator()(Device device)
  {
    this->Tracker.ResetDevice(device);
  }
};
}

VTKM_CONT
void RuntimeDeviceTracker::Reset()
{
  std::fill_n(this->Internals->RuntimeValid, VTKM_MAX_DEVICE_ADAPTER_ID, false);

  RuntimeDeviceTrackerResetFunctor functor(*this);
  vtkm::ListForEach(functor, VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG());
}

VTKM_CONT
vtkm::cont::RuntimeDeviceTracker RuntimeDeviceTracker::DeepCopy() const
{
  vtkm::cont::RuntimeDeviceTracker dest;
  dest.DeepCopy(*this);
  return dest;
}

VTKM_CONT
void RuntimeDeviceTracker::DeepCopy(const vtkm::cont::RuntimeDeviceTracker& src)
{
  std::copy_n(
    src.Internals->RuntimeValid, VTKM_MAX_DEVICE_ADAPTER_ID, this->Internals->RuntimeValid);
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

  std::fill_n(this->Internals->RuntimeValid, VTKM_MAX_DEVICE_ADAPTER_ID, false);

  this->Internals->RuntimeValid[deviceId.GetValue()] = runtimeExists;
}

VTKM_CONT
vtkm::cont::RuntimeDeviceTracker GetGlobalRuntimeDeviceTracker()
{
#if defined(VTKM_CLANG) && (__apple_build_version__ < 8000000)
  static std::mutex mtx;
  static std::map<std::thread::id, vtkm::cont::RuntimeDeviceTracker> globalTrackers;
  std::thread::id this_id = std::this_thread::get_id();

  std::unique_lock<std::mutex> lock(mtx);
  auto iter = globalTrackers.find(this_id);
  if (iter != globalTrackers.end())
  {
    return iter->second;
  }
  else
  {
    vtkm::cont::RuntimeDeviceTracker tracker;
    globalTrackers[this_id] = tracker;
    return tracker;
  }
#else
  return runtimeDeviceTracker;
#endif
}

VTKM_CONT
DeviceAdapterNameType RuntimeDeviceTracker::GetDeviceName(DeviceAdapterId device) const
{
  auto id = device.GetValue();

  if (id < 0)
  {
    switch (id)
    {
      case VTKM_DEVICE_ADAPTER_ERROR:
        return vtkm::cont::DeviceAdapterTraits<vtkm::cont::DeviceAdapterTagError>::GetName();
      case VTKM_DEVICE_ADAPTER_UNDEFINED:
        return vtkm::cont::DeviceAdapterTraits<vtkm::cont::DeviceAdapterTagUndefined>::GetName();
      default:
        break;
    }
  }
  else if (id >= VTKM_MAX_DEVICE_ADAPTER_ID)
  {
    switch (id)
    {
      case VTKM_DEVICE_ADAPTER_ANY:
        return vtkm::cont::DeviceAdapterTraits<vtkm::cont::DeviceAdapterTagAny>::GetName();
      default:
        break;
    }
  }
  else // id is valid:
  {
    return this->Internals->DeviceNames[id];
  }

  // Device 0 is invalid:
  return this->Internals->DeviceNames[0];
}
}
} // namespace vtkm::cont
