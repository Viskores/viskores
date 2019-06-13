//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/internal/brigand.hpp>

namespace
{
template <typename State, typename T>
struct RemoveDisabledDevice
{
  using type = typename std::conditional<T::IsEnabled, brigand::push_back<State, T>, State>::type;
};

/// TMP code to generate enabled device timer container
using AllDeviceList = vtkm::cont::DeviceAdapterListTagCommon::list;
using EnabledDeviceList = brigand::fold<AllDeviceList,
                                        brigand::list<>,
                                        RemoveDisabledDevice<brigand::_state, brigand::_element>>;
struct EnabledDeviceListTag : vtkm::ListTagBase<>
{
  using list = EnabledDeviceList;
};
using EnabledTimerImpls =
  brigand::transform<EnabledDeviceList,
                     brigand::bind<vtkm::cont::DeviceAdapterTimerImplementation, brigand::_1>>;
using EnabledTimerImplTuple = brigand::as_tuple<EnabledTimerImpls>;
} // anonymous namespace

namespace vtkm
{
namespace cont
{
namespace detail
{

class EnabledDeviceTimerImpls
{
public:
  EnabledDeviceTimerImpls() {}
  ~EnabledDeviceTimerImpls() {}
  // A tuple of enabled timer implementations
  EnabledTimerImplTuple timerImplTuple;
};
}
}
} // namespace vtkm::cont::detail

namespace
{

// C++11 does not support get tuple element by type. C++14 does support that.
// Get the index of a type in tuple elements
template <class T, class Tuple>
struct Index;

template <class T, template <typename...> class Container, class... Types>
struct Index<T, Container<T, Types...>>
{
  static const std::size_t value = 0;
};

template <class T, class U, template <typename...> class Container, class... Types>
struct Index<T, Container<U, Types...>>
{
  static const std::size_t value = 1 + Index<T, Container<Types...>>::value;
};

template <typename Device>
VTKM_CONT inline
  typename std::tuple_element<Index<Device, EnabledDeviceList>::value, EnabledTimerImplTuple>::type&
  GetTimerImpl(Device, vtkm::cont::detail::EnabledDeviceTimerImpls* timerImpls)
{
  return std::get<Index<Device, EnabledDeviceList>::value>(timerImpls->timerImplTuple);
}

template <typename Device>
VTKM_CONT inline const typename std::tuple_element<Index<Device, EnabledDeviceList>::value,
                                                   EnabledTimerImplTuple>::type&
GetTimerImpl(Device, const vtkm::cont::detail::EnabledDeviceTimerImpls* timerImpls)
{
  return std::get<Index<Device, EnabledDeviceList>::value>(timerImpls->timerImplTuple);
}

struct ResetFunctor
{
  template <typename Device>
  VTKM_CONT void operator()(Device device,
                            vtkm::cont::Timer* timer,
                            vtkm::cont::detail::EnabledDeviceTimerImpls* timerImpls)
  {
    if ((timer->GetDevice() == device) || (timer->GetDevice() == vtkm::cont::DeviceAdapterTagAny()))
    {
      GetTimerImpl(device, timerImpls).Reset();
    }
  }
};

struct StartFunctor
{
  template <typename Device>
  VTKM_CONT void operator()(Device device,
                            vtkm::cont::Timer* timer,
                            vtkm::cont::detail::EnabledDeviceTimerImpls* timerImpls)
  {
    if ((timer->GetDevice() == device) || (timer->GetDevice() == vtkm::cont::DeviceAdapterTagAny()))
    {
      GetTimerImpl(device, timerImpls).Start();
    }
  }
};

struct StopFunctor
{
  template <typename Device>
  VTKM_CONT void operator()(Device device,
                            vtkm::cont::Timer* timer,
                            vtkm::cont::detail::EnabledDeviceTimerImpls* timerImpls)
  {
    if ((timer->GetDevice() == device) || (timer->GetDevice() == vtkm::cont::DeviceAdapterTagAny()))
    {
      GetTimerImpl(device, timerImpls).Stop();
    }
  }
};

struct StartedFunctor
{
  bool Value = true;

  template <typename Device>
  VTKM_CONT void operator()(Device device,
                            const vtkm::cont::Timer* timer,
                            const vtkm::cont::detail::EnabledDeviceTimerImpls* timerImpls)
  {
    if ((timer->GetDevice() == device) || (timer->GetDevice() == vtkm::cont::DeviceAdapterTagAny()))
    {
      this->Value &= GetTimerImpl(device, timerImpls).Started();
    }
  }
};

struct StoppedFunctor
{
  bool Value = true;

  template <typename Device>
  VTKM_CONT void operator()(Device device,
                            const vtkm::cont::Timer* timer,
                            const vtkm::cont::detail::EnabledDeviceTimerImpls* timerImpls)
  {
    if ((timer->GetDevice() == device) || (timer->GetDevice() == vtkm::cont::DeviceAdapterTagAny()))
    {
      this->Value &= GetTimerImpl(device, timerImpls).Stopped();
    }
  }
};

struct ReadyFunctor
{
  bool Value = true;

  template <typename Device>
  VTKM_CONT void operator()(Device device,
                            const vtkm::cont::Timer* timer,
                            const vtkm::cont::detail::EnabledDeviceTimerImpls* timerImpls)
  {
    if ((timer->GetDevice() == device) || (timer->GetDevice() == vtkm::cont::DeviceAdapterTagAny()))
    {
      this->Value &= GetTimerImpl(device, timerImpls).Ready();
    }
  }
};

struct ElapsedTimeFunctor
{
  vtkm::Float64 ElapsedTime = 0.0;

  template <typename Device>
  VTKM_CONT void operator()(Device deviceToTry,
                            vtkm::cont::DeviceAdapterId deviceToRunOn,
                            const vtkm::cont::detail::EnabledDeviceTimerImpls* timerImpls)
  {
    if ((deviceToRunOn == deviceToTry) || (deviceToRunOn == vtkm::cont::DeviceAdapterTagAny()))
    {
      this->ElapsedTime =
        vtkm::Max(this->ElapsedTime, GetTimerImpl(deviceToTry, timerImpls).GetElapsedTime());
    }
  }
};
} // anonymous namespace

namespace vtkm
{
namespace cont
{

Timer::Timer()
  : Device(vtkm::cont::DeviceAdapterTagAny())
  , Internal(nullptr)
{
  this->Init();
}

Timer::Timer(vtkm::cont::DeviceAdapterId device)
  : Device(device)
  , Internal(nullptr)
{
  const vtkm::cont::RuntimeDeviceTracker& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  if (device != DeviceAdapterTagAny() && !tracker.CanRunOn(device))
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "Device '" << device.GetName() << "' can not run on current Device."
                                                 "Thus timer is not usable");
  }

  this->Init();
}


Timer::~Timer() = default;

void Timer::Init()
{
  if (!this->Internal)
  {
    this->Internal.reset(new detail::EnabledDeviceTimerImpls);
  }
}

void Timer::Reset()
{
  vtkm::ListForEach(ResetFunctor(), EnabledDeviceListTag(), this, this->Internal.get());
}

void Timer::Reset(vtkm::cont::DeviceAdapterId device)
{
  const vtkm::cont::RuntimeDeviceTracker& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  if (device != DeviceAdapterTagAny() && !tracker.CanRunOn(device))
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "Device '" << device.GetName() << "' can not run on current Device."
                                                 "Thus timer is not usable");
  }

  this->Device = device;
  this->Reset();
}

void Timer::Start()
{
  vtkm::ListForEach(StartFunctor(), EnabledDeviceListTag(), this, this->Internal.get());
}

void Timer::Stop()
{
  vtkm::ListForEach(StopFunctor(), EnabledDeviceListTag(), this, this->Internal.get());
}

bool Timer::Started() const
{
  StartedFunctor functor;
  vtkm::ListForEach(functor, EnabledDeviceListTag(), this, this->Internal.get());
  return functor.Value;
}

bool Timer::Stopped() const
{
  StoppedFunctor functor;
  vtkm::ListForEach(functor, EnabledDeviceListTag(), this, this->Internal.get());
  return functor.Value;
}

bool Timer::Ready() const
{
  ReadyFunctor functor;
  vtkm::ListForEach(functor, EnabledDeviceListTag(), this, this->Internal.get());
  return functor.Value;
}

vtkm::Float64 Timer::GetElapsedTime(vtkm::cont::DeviceAdapterId device) const
{
  vtkm::cont::DeviceAdapterId deviceToTime = device;

  if (this->Device != DeviceAdapterTagAny())
  {
    // Timer is constructed for a specific device. Only querying on this device is allowed.
    if (deviceToTime == vtkm::cont::DeviceAdapterTagAny())
    {
      // User did not specify a device to time on. Use the one set in the timer.
      deviceToTime = this->Device;
    }
    else if (deviceToTime == this->Device)
    {
      // User asked for the same device already set for the timer. We are OK. Nothing to do.
    }
    else
    {
      // The user selected a device that is differnt than the one set for the timer. This query
      // is not allowed.
      VTKM_LOG_S(vtkm::cont::LogLevel::Error,
                 "Device '" << device.GetName() << "' is not supported for current timer"
                            << "("
                            << this->Device.GetName()
                            << ")");
      return 0.0;
    }
  }

  // If we have specified a specific device, make sure we can run on it.
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  if (deviceToTime != vtkm::cont::DeviceAdapterTagAny() && !tracker.CanRunOn(deviceToTime))
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "Device '" << deviceToTime.GetName() << "' can not run on current Device."
                                                       " Thus timer is not usable");
    return 0.0;
  }

  ElapsedTimeFunctor functor;
  vtkm::ListForEach(functor, EnabledDeviceListTag(), deviceToTime, this->Internal.get());

  return functor.ElapsedTime;
}
}
} // namespace vtkm::cont
