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
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorBadValue.h>

#include <vtkm/cont/RuntimeDeviceTracker.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

template <typename Functor, typename Device, bool DeviceAdapterValid>
struct TryExecuteRunIfValid;

template <typename Functor, typename Device>
struct TryExecuteRunIfValid<Functor, Device, false>
{
  VTKM_CONT
  static bool Run(Functor&, const vtkm::cont::RuntimeDeviceTracker&) { return false; }
};

template <typename Functor, typename Device>
struct TryExecuteRunIfValid<Functor, Device, true>
{
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  VTKM_CONT
  static bool Run(Functor& functor, vtkm::cont::RuntimeDeviceTracker tracker)
  {
    if (tracker.CanRunOn(Device()))
    {
      try
      {
        return functor(Device());
      }
      catch (vtkm::cont::ErrorBadAllocation& e)
      {
        std::cerr << "caught ErrorBadAllocation " << e.GetMessage() << std::endl;
        //currently we only consider OOM errors worth disabling a device for
        //than we fallback to another device
        tracker.ReportAllocationFailure(Device(), e);
      }
      catch (vtkm::cont::ErrorBadType& e)
      {
        //should bad type errors should stop the execution, instead of
        //deferring to another device adapter?
        std::cerr << "caught ErrorBadType : " << e.GetMessage() << std::endl;
      }
      catch (vtkm::cont::ErrorBadValue& e)
      {
        //should bad value errors should stop the filter, instead of deferring
        //to another device adapter?
        std::cerr << "caught ErrorBadValue : " << e.GetMessage() << std::endl;
      }
      catch (vtkm::cont::Error& e)
      {
        //general errors should be caught and let us try the next device adapter.
        std::cerr << "exception is: " << e.GetMessage() << std::endl;
      }
      catch (std::exception& e)
      {
        std::cerr << "caught standard exception: " << e.what() << std::endl;
      }
      catch (...)
      {
        std::cerr << "unknown exception caught" << std::endl;
      }
    }

    // If we are here, then the functor was either never run or failed.
    return false;
  }
};

template <typename FunctorType>
struct TryExecuteImpl
{
  // Warning, these are a references. Make sure referenced objects do not go
  // out of scope.
  FunctorType& Functor;
  vtkm::cont::RuntimeDeviceTracker Tracker;

  bool Success;

  VTKM_CONT
  TryExecuteImpl(
    FunctorType& functor,
    vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())
    : Functor(functor)
    , Tracker(tracker)
    , Success(false)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    if (!this->Success)
    {
      using DeviceTraits = vtkm::cont::DeviceAdapterTraits<Device>;

      this->Success = detail::TryExecuteRunIfValid<FunctorType, Device, DeviceTraits::Valid>::Run(
        this->Functor, this->Tracker);
    }

    return this->Success;
  }

private:
  void operator=(const TryExecuteImpl<FunctorType>&) = delete;
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
VTKM_CONT bool TryExecute(const Functor& functor,
                          vtkm::cont::RuntimeDeviceTracker tracker,
                          DeviceList)
{
  detail::TryExecuteImpl<const Functor> internals(functor, tracker);
  vtkm::ListForEach(internals, DeviceList());
  return internals.Success;
}
template <typename Functor, typename DeviceList>
VTKM_CONT bool TryExecute(Functor& functor, vtkm::cont::RuntimeDeviceTracker tracker, DeviceList)
{
  detail::TryExecuteImpl<Functor> internals(functor, tracker);
  vtkm::ListForEach(internals, DeviceList());
  return internals.Success;
}
template <typename Functor, typename DeviceList>
VTKM_CONT bool TryExecute(const Functor& functor, DeviceList)
{
  return vtkm::cont::TryExecute(functor, vtkm::cont::GetGlobalRuntimeDeviceTracker(), DeviceList());
}
template <typename Functor, typename DeviceList>
VTKM_CONT bool TryExecute(Functor& functor, DeviceList)
{
  return vtkm::cont::TryExecute(functor, vtkm::cont::GetGlobalRuntimeDeviceTracker(), DeviceList());
}
template <typename Functor>
VTKM_CONT bool TryExecute(
  const Functor& functor,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())
{
  return vtkm::cont::TryExecute(functor, tracker, VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG());
}
template <typename Functor>
VTKM_CONT bool TryExecute(
  Functor& functor,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())
{
  return vtkm::cont::TryExecute(functor, tracker, VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG());
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_TryExecute_h
