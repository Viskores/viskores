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
#ifndef vtk_m_cont_TryExecute_h
#define vtk_m_cont_TryExecute_h

#include <vtkm/cont/DeviceAdapterListTag.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

VTKM_CONT_EXPORT void HandleTryExecuteException(vtkm::cont::DeviceAdapterId,
                                                const std::string&,
                                                vtkm::cont::RuntimeDeviceTracker&);

template <typename DeviceTag, typename Functor, typename... Args>
inline bool TryExecuteIfValid(std::true_type,
                              DeviceTag tag,
                              Functor&& f,
                              vtkm::cont::DeviceAdapterId devId,
                              vtkm::cont::RuntimeDeviceTracker& tracker,
                              Args&&... args)
{
  if ((tag == devId || devId == DeviceAdapterIdAny()) && tracker.CanRunOn(tag))
  {
    try
    {
      return f(tag, std::forward<Args>(args)...);
    }
    catch (...)
    {
      using Traits = vtkm::cont::DeviceAdapterTraits<DeviceTag>;
      detail::HandleTryExecuteException(tag, Traits::GetName(), tracker);
    }
  }

  // If we are here, then the functor was either never run or failed.
  return false;
}

template <typename DeviceTag, typename Functor, typename... Args>
inline bool TryExecuteIfValid(std::false_type,
                              DeviceTag,
                              Functor&&,
                              vtkm::cont::DeviceAdapterId,
                              vtkm::cont::RuntimeDeviceTracker&,
                              Args&&...)
{
  return false;
}

struct TryExecuteWrapper
{
  template <typename DeviceTag, typename Functor, typename... Args>
  inline void operator()(DeviceTag tag,
                         Functor&& f,
                         vtkm::cont::DeviceAdapterId devId,
                         vtkm::cont::RuntimeDeviceTracker& tracker,
                         bool& ran,
                         Args&&... args) const
  {
    if (!ran)
    {
      ran = TryExecuteIfValid(std::integral_constant<bool, DeviceTag::IsEnabled>(),
                              tag,
                              std::forward<Functor>(f),
                              devId,
                              std::forward<decltype(tracker)>(tracker),
                              std::forward<Args>(args)...);
    }
  }
};

template <typename Functor, typename DeviceList, typename... Args>
inline bool TryExecuteImpl(vtkm::cont::DeviceAdapterId devId,
                           Functor&& functor,
                           std::true_type,
                           std::true_type,
                           vtkm::cont::RuntimeDeviceTracker& tracker,
                           DeviceList list,
                           Args&&... args)
{
  bool success = false;
  TryExecuteWrapper task;
  vtkm::ListForEach(task,
                    list,
                    std::forward<Functor>(functor),
                    devId,
                    tracker,
                    success,
                    std::forward<Args>(args)...);
  return success;
}

template <typename Functor, typename... Args>
inline bool TryExecuteImpl(vtkm::cont::DeviceAdapterId devId,
                           Functor&& functor,
                           std::false_type,
                           std::false_type,
                           Args&&... args)
{
  bool success = false;
  auto tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker();
  TryExecuteWrapper task;
  vtkm::ListForEach(task,
                    VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG(),
                    std::forward<Functor>(functor),
                    devId,
                    tracker,
                    success,
                    std::forward<Args>(args)...);
  return success;
}

template <typename Functor, typename Arg1, typename... Args>
inline bool TryExecuteImpl(vtkm::cont::DeviceAdapterId devId,
                           Functor&& functor,
                           std::true_type t,
                           std::false_type,
                           Arg1&& arg1,
                           Args&&... args)
{
  return TryExecuteImpl(devId,
                        std::forward<Functor>(functor),
                        t,
                        t,
                        std::forward<Arg1>(arg1),
                        VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG(),
                        std::forward<Args>(args)...);
}

template <typename Functor, typename Arg1, typename... Args>
inline bool TryExecuteImpl(vtkm::cont::DeviceAdapterId devId,
                           Functor&& functor,
                           std::false_type,
                           std::true_type t,
                           Arg1&& arg1,
                           Args&&... args)
{
  auto tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker();
  return TryExecuteImpl(devId,
                        std::forward<Functor>(functor),
                        t,
                        t,
                        tracker,
                        std::forward<Arg1>(arg1),
                        std::forward<Args>(args)...);
}

} // namespace detail

///@{
/// \brief Try to execute a functor on a specific device selected at runtime.
///
/// This function takes a functor and a \c DeviceAdapterId which represents a
/// specific device to attempt to run on at runtime. It also optionally accepts
/// the following parameters:
/// - A set of devices to compile support for
/// - \c RuntimeDeviceTracker which holds which devices have been enabled at runtime, and
///   records any functor execution failures
///
/// It then iterates over the set of devices finding which one matches the provided
/// adapter Id and is also enabled in the runtime. The function will return true
/// only if the device adapter was valid, and the task was successfully run.
/// The optional \c RuntimeDeviceTracker allows for monitoring for certain
/// failures across calls to TryExecute and skip trying devices with a history of failure.
///
/// The TryExecuteOnDevice is also able to perfectly forward arbitrary arguments onto the functor.
/// These arguments must be placed after the optional \c RuntimeDeviceTracker, and device adapter
/// list and will passed to the functor in the same order as listed.
///
/// The functor must implement the function call operator ( \c operator() ) with a return type of
/// \c bool and that is \c true if the execution succeeds, \c false if it fails. If an exception
/// is thrown from the functor, then the execution is assumed to have failed. The functor call
/// operator must also take at least one argument being the required \c DeviceAdapterTag to use.
///
/// \code{.cpp}
/// struct TryCallExample
/// {
///   template<typename DeviceList>
///   bool operator()(DeviceList tags, int) const
///   {
///     return true;
///   }
/// };
///
///
/// // Execute only on the device which corresponds to devId
/// // Will not execute all if devId is
/// vtkm::cont::TryExecuteOnDevice(devId, TryCallExample(), int{42});
///
/// // Executing on a specific deviceID with a runtime tracker
/// auto tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker();
/// vtkm::cont::TryExecute(devId, TryCallExample(), tracker, int{42});
///
/// \endcode
///
/// This function returns \c true if the functor succeeded on a device,
/// \c false otherwise.
///
/// If no device list is specified, then \c VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG
/// is used.
///
/// If no \c RuntimeDeviceTracker specified, then \c GetGlobalRuntimeDeviceTracker()
/// is used.
template <typename Functor>
VTKM_CONT bool TryExecuteOnDevice(vtkm::cont::DeviceAdapterId devId, Functor&& functor)
{
  //we haven't been passed either a runtime tracker or a device list
  return detail::TryExecuteImpl(
    devId, std::forward<Functor>(functor), std::false_type{}, std::false_type{});
}
template <typename Functor, typename Arg1>
VTKM_CONT bool TryExecuteOnDevice(vtkm::cont::DeviceAdapterId devId, Functor&& functor, Arg1&& arg1)
{
  //determine if we are being passed a device adapter or runtime tracker as our argument
  using is_deviceAdapter = typename std::is_base_of<vtkm::detail::ListRoot, Arg1>::type;
  using is_tracker = typename std::is_base_of<vtkm::cont::RuntimeDeviceTracker,
                                              typename std::remove_reference<Arg1>::type>::type;

  return detail::TryExecuteImpl(devId,
                                std::forward<Functor>(functor),
                                is_tracker{},
                                is_deviceAdapter{},
                                std::forward<Arg1>(arg1));
}
template <typename Functor, typename Arg1, typename Arg2, typename... Args>
VTKM_CONT bool TryExecuteOnDevice(vtkm::cont::DeviceAdapterId devId,
                                  Functor&& functor,
                                  Arg1&& arg1,
                                  Arg2&& arg2,
                                  Args&&... args)
{
  //So arg1 can be runtime or device adapter
  //if arg1 is runtime, we need to see if arg2 is device adapter
  using is_arg1_tracker =
    typename std::is_base_of<vtkm::cont::RuntimeDeviceTracker,
                             typename std::remove_reference<Arg1>::type>::type;
  using is_arg2_devicelist = typename std::is_base_of<vtkm::detail::ListRoot, Arg2>::type;

  //We now know what of three states we are currently at
  using has_runtime_and_deviceAdapter =
    brigand::bool_<is_arg1_tracker::value && is_arg2_devicelist::value>;
  using has_just_runtime = brigand::bool_<is_arg1_tracker::value && !is_arg2_devicelist::value>;
  using has_just_devicelist = typename std::is_base_of<vtkm::detail::ListRoot, Arg1>::type;

  //With this information we can now compute if we have a runtime tracker and/or
  //the device adapter and enable the correct flags
  using first_true =
    brigand::bool_<has_runtime_and_deviceAdapter::value || has_just_runtime::value>;
  using second_true =
    brigand::bool_<has_runtime_and_deviceAdapter::value || has_just_devicelist::value>;

  return detail::TryExecuteImpl(devId,
                                functor,
                                first_true{},
                                second_true{},
                                std::forward<Arg1>(arg1),
                                std::forward<Arg2>(arg2),
                                std::forward<Args>(args)...);
}
//@} //block doxygen all TryExecuteOnDevice functions

///@{
/// \brief Try to execute a functor on a set of devices until one succeeds.
///
/// This function takes a functor and optionally the following:
/// - A set of devices to compile support for
/// - \c RuntimeDeviceTracker which holds which devices have been enabled at runtime, and
///   records any functor execution failures
///
/// It then tries to run the functor for each device (in the order given in the list) until the
/// execution succeeds. The optional \c RuntimeDeviceTracker allows for monitoring for certain
/// failures across calls to TryExecute and skip trying devices with a history of failure.
///
/// The TryExecute is also able to perfectly forward arbitrary arguments onto the functor.
/// These arguments must be placed after the optional \c RuntimeDeviceTracker, and device adapter
/// list and will passed to the functor in the same order as listed.
///
/// The functor must implement the function call operator ( \c operator() ) with a return type of
/// \c bool and that is \c true if the execution succeeds, \c false if it fails. If an exception
/// is thrown from the functor, then the execution is assumed to have failed. The functor call
/// operator must also take at least one argument being the required \c DeviceAdapterTag to use.
///
/// \code{.cpp}
/// struct TryCallExample
/// {
///   template<typename DeviceList>
///   bool operator()(DeviceList tags, int) const
///   {
///     return true;
///   }
/// };
///
///
/// // Executing without a runtime tracker, deviceId, or device list
/// vtkm::cont::TryExecute(TryCallExample(), int{42});
///
/// // Executing with a runtime tracker
/// auto tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker();
/// vtkm::cont::TryExecute(TryCallExample(), tracker, int{42});
///
/// // Executing with a device list
/// using DeviceList = vtkm::ListTagBase<vtkm::cont::DeviceAdapterTagSerial>;
/// vtkm::cont::TryExecute(TryCallExample(), DeviceList(), int{42});
///
/// // Executing with a runtime tracker and device list
/// vtkm::cont::TryExecute(EdgeCaseFunctor(), tracker, DeviceList(), int{42});
///
/// \endcode
///
/// This function returns \c true if the functor succeeded on a device,
/// \c false otherwise.
///
/// If no device list is specified, then \c VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG
/// is used.
///
/// If no \c RuntimeDeviceTracker specified, then \c GetGlobalRuntimeDeviceTracker()
/// is used.
template <typename Functor, typename... Args>
VTKM_CONT bool TryExecute(Functor&& functor, Args&&... args)
{
  return TryExecuteOnDevice(
    vtkm::cont::DeviceAdapterIdAny(), std::forward<Functor>(functor), std::forward<Args>(args)...);
}


//@} //block doxygen all TryExecute functions
}
} // namespace vtkm::cont

#endif //vtk_m_cont_TryExecute_h
