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

namespace vtkm
{
namespace cont
{

namespace detail
{

VTKM_CONT_EXPORT void HandleTryExecuteException(vtkm::Int8,
                                                const std::string&,
                                                vtkm::cont::RuntimeDeviceTracker&);

template <typename DeviceTag, typename Functor>
bool TryExecuteIfValid(std::true_type,
                       DeviceTag tag,
                       Functor&& f,
                       vtkm::cont::RuntimeDeviceTracker& tracker)
{
  if (tracker.CanRunOn(tag))
  {
    try
    {
      return f(tag);
    }
    catch (...)
    {
      using Traits = vtkm::cont::DeviceAdapterTraits<DeviceTag>;
      HandleTryExecuteException(Traits::GetId(), Traits::GetName(), tracker);
    }
  }

  // If we are here, then the functor was either never run or failed.
  return false;
}

template <typename DeviceTag, typename Functor>
bool TryExecuteIfValid(std::false_type, DeviceTag, Functor&&, vtkm::cont::RuntimeDeviceTracker&)
{
  return false;
}

struct TryExecuteImpl
{
  template <typename DeviceTag, typename Functor>
  void operator()(DeviceTag tag,
                  Functor&& f,
                  vtkm::cont::RuntimeDeviceTracker& tracker,
                  bool& ran) const
  {
    if (!ran)
    {
      using DeviceTraits = vtkm::cont::DeviceAdapterTraits<DeviceTag>;
      ran = TryExecuteIfValid(std::integral_constant<bool, DeviceTraits::Valid>(),
                              tag,
                              std::forward<Functor>(f),
                              tracker);
    }
  }
};

} // namespace detail

/// \brief Try to execute a functor on a list of devices until one succeeds.
///
/// This function takes a functor and a list of devices. It then tries to run
/// the functor for each device (in the order given in the list) until the
/// execution succeeds.
///
/// The functor parentheses operator should take exactly one argument, which is
/// the \c DeviceAdapterTag to use. The functor should return a \c bool that is
/// \c true if the execution succeeds, \c false if it fails. If an exception is
/// thrown from the functor, then the execution is assumed to have failed.
///
/// This function also optionally takes a \c RuntimeDeviceTracker, which will
/// monitor for certain failures across calls to TryExecute and skip trying
/// devices with a history of failure.
///
/// This function returns \c true if the functor succeeded on a device,
/// \c false otherwise.
///
/// If no device list is specified, then \c VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG
/// is used.
///
template <typename Functor, typename DeviceList>
VTKM_CONT bool TryExecute(Functor&& functor, vtkm::cont::RuntimeDeviceTracker tracker, DeviceList)
{
  bool success = false;
  detail::TryExecuteImpl task;
  vtkm::ListForEach(task, DeviceList(), std::forward<Functor>(functor), tracker, success);
  return success;
}
template <typename Functor, typename DeviceList>
VTKM_CONT bool TryExecute(Functor&& functor, DeviceList)
{
  return vtkm::cont::TryExecute(functor, vtkm::cont::GetGlobalRuntimeDeviceTracker(), DeviceList());
}
template <typename Functor>
VTKM_CONT bool TryExecute(
  Functor&& functor,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())
{
  return vtkm::cont::TryExecute(
    functor, std::forward<decltype(tracker)>(tracker), VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG());
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_TryExecute_h
