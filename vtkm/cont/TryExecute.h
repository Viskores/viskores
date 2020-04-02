//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_TryExecute_h
#define vtk_m_cont_TryExecute_h

#include <vtkm/cont/DeviceAdapterList.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>


namespace vtkm
{
namespace cont
{

namespace detail
{

VTKM_CONT_EXPORT void HandleTryExecuteException(vtkm::cont::DeviceAdapterId,
                                                vtkm::cont::RuntimeDeviceTracker&,
                                                const std::string& functorName);

template <typename DeviceTag, typename Functor, typename... Args>
inline bool TryExecuteIfValid(std::true_type,
                              DeviceTag tag,
                              Functor&& f,
                              vtkm::cont::DeviceAdapterId devId,
                              vtkm::cont::RuntimeDeviceTracker& tracker,
                              Args&&... args)
{
  if ((tag == devId || devId == DeviceAdapterTagAny()) && tracker.CanRunOn(tag))
  {
    try
    {
      return f(tag, std::forward<Args>(args)...);
    }
    catch (...)
    {
      detail::HandleTryExecuteException(tag, tracker, vtkm::cont::TypeToString<Functor>());
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
                           DeviceList list,
                           Args&&... args)
{
  bool success = false;
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
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
                           Args&&... args)
{
  bool success = false;
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  TryExecuteWrapper task;
  vtkm::ListForEach(task,
                    VTKM_DEFAULT_DEVICE_ADAPTER_LIST(),
                    std::forward<Functor>(functor),
                    devId,
                    tracker,
                    success,
                    std::forward<Args>(args)...);
  return success;
}
} // namespace detail

///@{
/// \brief Try to execute a functor on a specific device selected at runtime.
///
/// This function takes a functor and a \c DeviceAdapterId which represents a
/// specific device to attempt to run on at runtime. It also optionally accepts
/// a set of devices to compile support for.
///
/// It then iterates over the set of devices finding which one matches the provided
/// adapter Id and is also enabled in the runtime. The function will return true
/// only if the device adapter was valid, and the task was successfully run.
///
/// The TryExecuteOnDevice is also able to perfectly forward arbitrary arguments onto the functor.
/// These arguments must be placed after the optional device adapter list and will passed to
/// the functor in the same order as listed.
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
/// \endcode
///
/// This function returns \c true if the functor succeeded on a device,
/// \c false otherwise.
///
/// If no device list is specified, then \c VTKM_DEFAULT_DEVICE_ADAPTER_LIST
/// is used.
///
template <typename Functor>
VTKM_CONT bool TryExecuteOnDevice(vtkm::cont::DeviceAdapterId devId, Functor&& functor)
{
  //we haven't been passed either a runtime tracker or a device list
  return detail::TryExecuteImpl(devId, std::forward<Functor>(functor), std::false_type{});
}
template <typename Functor, typename Arg1, typename... Args>
VTKM_CONT bool TryExecuteOnDevice(vtkm::cont::DeviceAdapterId devId,
                                  Functor&& functor,
                                  Arg1&& arg1,
                                  Args&&... args)
{
  //determine if we are being passed a device adapter or runtime tracker as our argument
  using is_deviceAdapter = vtkm::internal::IsList<Arg1>;

  return detail::TryExecuteImpl(devId,
                                std::forward<Functor>(functor),
                                is_deviceAdapter{},
                                std::forward<Arg1>(arg1),
                                std::forward<Args>(args)...);
}

//@} //block doxygen all TryExecuteOnDevice functions

///@{
/// \brief Try to execute a functor on a set of devices until one succeeds.
///
/// This function takes a functor and optionally a set of devices to compile support.
/// It then tries to run the functor for each device (in the order given in the list) until the
/// execution succeeds.
///
/// The TryExecute is also able to perfectly forward arbitrary arguments onto the functor.
/// These arguments must be placed after the optional device adapter list and will passed
///  to the functor in the same order as listed.
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
/// // Executing without a deviceId, or device list
/// vtkm::cont::TryExecute(TryCallExample(), int{42});
///
/// // Executing with a device list
/// using DeviceList = vtkm::List<vtkm::cont::DeviceAdapterTagSerial>;
/// vtkm::cont::TryExecute(TryCallExample(), DeviceList(), int{42});
///
/// \endcode
///
/// This function returns \c true if the functor succeeded on a device,
/// \c false otherwise.
///
/// If no device list is specified, then \c VTKM_DEFAULT_DEVICE_ADAPTER_LIST
/// is used.
///
template <typename Functor, typename... Args>
VTKM_CONT bool TryExecute(Functor&& functor, Args&&... args)
{
  return TryExecuteOnDevice(
    vtkm::cont::DeviceAdapterTagAny(), std::forward<Functor>(functor), std::forward<Args>(args)...);
}


//@} //block doxygen all TryExecute functions
}
} // namespace vtkm::cont

#endif //vtk_m_cont_TryExecute_h
