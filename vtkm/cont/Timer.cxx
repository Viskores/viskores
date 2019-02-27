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
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/internal/brigand.hpp>

namespace vtkm
{
namespace cont
{

namespace detail
{
template <typename State, typename T>
struct RemoveDisabledDevice
{
  using type = typename std::conditional<T::IsEnabled, brigand::push_back<State, T>, State>::type;
};

/// TMP code to generate enabled device timer container
using AllDeviceList = DeviceAdapterListTagCommon::list;
using EnabledDeviceList =
  brigand::fold<AllDeviceList,
                brigand::list<>,
                detail::RemoveDisabledDevice<brigand::_state, brigand::_element>>;
struct EnabledDeviceListTag : vtkm::ListTagBase<>
{
  using list = EnabledDeviceList;
};
using EnabledTimerImpls =
  brigand::transform<EnabledDeviceList,
                     brigand::bind<DeviceAdapterTimerImplementation, brigand::_1>>;
using EnabledTimerImplTuple = brigand::as_tuple<EnabledTimerImpls>;
}

enum class TimerDispatchTag : int
{
  Reset,
  Start,
  Stop,
  Started,
  Stopped,
  Ready,
  GetElapsedTime
};

class EnabledDeviceTimerImpls
{
public:
  EnabledDeviceTimerImpls() {}
  ~EnabledDeviceTimerImpls() {}
  // A tuple of enabled timer implementations
  detail::EnabledTimerImplTuple timerImplTuple;
};

namespace detail
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

struct TimerFunctor
{
  TimerFunctor()
    : elapsedTime(0)
    , value(true)
  {
  }

  template <typename DeviceAdapter>
  VTKM_CONT void operator()(DeviceAdapter device, Timer* timer, TimerDispatchTag tag)
  {
    if (timer->Device == device || timer->Device == DeviceAdapterTagAny())
    {
      auto& timerImpl = std::get<Index<DeviceAdapter, detail::EnabledDeviceList>::value>(
        timer->Internal->timerImplTuple);
      switch (tag)
      {
        case TimerDispatchTag::Reset:
          timerImpl.Reset();
          break;
        case TimerDispatchTag::Start:
          timerImpl.Start();
          break;
        case TimerDispatchTag::Stop:
          timerImpl.Stop();
          break;
        case TimerDispatchTag::Started:
          value &= timerImpl.Started();
          break;
        case TimerDispatchTag::Stopped:
          value &= timerImpl.Stopped();
          break;
        case TimerDispatchTag::Ready:
          value &= timerImpl.Ready();
          break;
        case TimerDispatchTag::GetElapsedTime:
        {
          if (timer->Device == DeviceAdapterTagAny() &&
              timer->DeviceForQuery == DeviceAdapterTagAny())
          { // Just want to do timing jobs
            elapsedTime = std::max(elapsedTime, timerImpl.GetElapsedTime());
            break;
          }
          else if (timer->Device == DeviceAdapterTagAny() && timer->DeviceForQuery == device)
          { // Given a generic timer, querying for a specific device time
            elapsedTime = timerImpl.GetElapsedTime();
            break;
          }
          else if (timer->Device == device && (timer->DeviceForQuery == DeviceAdapterTagAny() ||
                                               timer->Device == timer->DeviceForQuery))
          { // Given a specific timer, querying its elapsed time
            elapsedTime = timerImpl.GetElapsedTime();
            break;
          }
          break;
        }
      }
    }
  }

  vtkm::Float64 elapsedTime;
  bool value;
};
}


Timer::Timer()
  : Device(vtkm::cont::DeviceAdapterTagAny())
  , DeviceForQuery(vtkm::cont::DeviceAdapterTagAny())
  , Internal(nullptr)
{
  this->Init();
}

Timer::Timer(vtkm::cont::DeviceAdapterId device)
  : Device(device)
  , DeviceForQuery(vtkm::cont::DeviceAdapterTagAny())
  , Internal(nullptr)
{
  vtkm::cont::RuntimeDeviceTracker tracker;
  if (device != DeviceAdapterTagAny() && !tracker.CanRunOn(device))
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "Device '" << device.GetName() << "' can not run on current Device."
                                                 "Thus timer is not usable");
  }

  this->Init();
}


Timer::~Timer()
{
  delete this->Internal;
}

void Timer::Init()
{
  if (!this->Internal)
  {
    this->Internal = new EnabledDeviceTimerImpls();
  }
}

void Timer::Reset()
{
  detail::TimerFunctor functor;
  vtkm::ListForEach(functor, detail::EnabledDeviceListTag(), this, TimerDispatchTag::Reset);
}

void Timer::Start()
{

  detail::TimerFunctor functor;
  vtkm::ListForEach(functor, detail::EnabledDeviceListTag(), this, TimerDispatchTag::Start);
}

void Timer::Stop()
{
  detail::TimerFunctor functor;
  vtkm::ListForEach(functor, detail::EnabledDeviceListTag(), this, TimerDispatchTag::Stop);
}

bool Timer::Started()
{
  detail::TimerFunctor functor;
  vtkm::ListForEach(functor, detail::EnabledDeviceListTag(), this, TimerDispatchTag::Started);
  return functor.value;
}

bool Timer::Stopped()
{
  detail::TimerFunctor functor;
  vtkm::ListForEach(functor, detail::EnabledDeviceListTag(), this, TimerDispatchTag::Stopped);
  return functor.value;
}

bool Timer::Ready()
{
  detail::TimerFunctor functor;
  vtkm::ListForEach(functor, detail::EnabledDeviceListTag(), this, TimerDispatchTag::Ready);
  return functor.value;
}

vtkm::Float64 Timer::GetElapsedTime(DeviceAdapterId id)
{
  // Timer is constructed with a specific device. Only querying this device is allowed.
  if (this->Device != DeviceAdapterTagAny() && (id != DeviceAdapterTagAny() && this->Device != id))
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "Device '" << id.GetName() << "' is not supported for current timer"
                          << "("
                          << this->Device.GetName()
                          << ")");
    return 0.0;
  }

  // Timer is constructed with any device. Only querying enabled device is allowed.
  vtkm::cont::RuntimeDeviceTracker tracker;
  if (this->Device == DeviceAdapterTagAny() &&
      (id != DeviceAdapterTagAny() && !tracker.CanRunOn(id)))
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "Device '" << id.GetName() << "' can not run on current Device."
                                             "Thus timer is not usable");
    return 0.0;
  }

  this->DeviceForQuery = id;
  detail::TimerFunctor functor;
  vtkm::ListForEach(
    functor, detail::EnabledDeviceListTag(), this, TimerDispatchTag::GetElapsedTime);

  return functor.elapsedTime;
}
}
} // namespace vtkm::cont
