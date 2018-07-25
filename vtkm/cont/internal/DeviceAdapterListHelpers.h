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
#ifndef vtk_m_cont_internal_DeviceAdapterListHelpers_h
#define vtk_m_cont_internal_DeviceAdapterListHelpers_h

#include <vtkm/ListTag.h>
#include <vtkm/cont/ErrorBadDevice.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

//============================================================================
template <typename FunctorType>
class ExecuteIfValidDeviceTag
{
private:
  template <typename DeviceAdapter>
  using EnableIfValid = std::enable_if<DeviceAdapter::IsEnabled>;

  template <typename DeviceAdapter>
  using EnableIfInvalid = std::enable_if<!DeviceAdapter::IsEnabled>;

public:
  explicit ExecuteIfValidDeviceTag(const FunctorType& functor)
    : Functor(functor)
  {
  }

  template <typename DeviceAdapter, typename... Args>
  typename EnableIfValid<DeviceAdapter>::type operator()(
    DeviceAdapter device,
    const vtkm::cont::RuntimeDeviceTracker& tracker,
    Args&&... args) const
  {
    if (tracker.CanRunOn(device))
    {
      this->Functor(device, std::forward<Args>(args)...);
    }
  }

  // do not generate code for invalid devices
  template <typename DeviceAdapter, typename... Args>
  typename EnableIfInvalid<DeviceAdapter>::type operator()(DeviceAdapter,
                                                           const vtkm::cont::RuntimeDeviceTracker&,
                                                           Args&&...) const
  {
  }

private:
  FunctorType Functor;
};

/// Execute the given functor on each valid device in \c DeviceList.
///
template <typename DeviceList, typename Functor, typename... Args>
VTKM_CONT void ForEachValidDevice(DeviceList devices, const Functor& functor, Args&&... args)
{
  auto tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker();

  ExecuteIfValidDeviceTag<Functor> wrapped(functor);
  vtkm::ListForEach(wrapped, devices, tracker, std::forward<Args>(args)...);
}

//============================================================================
template <typename FunctorType>
class ExecuteIfSameDeviceId
{
public:
  ExecuteIfSameDeviceId(FunctorType functor)
    : Functor(functor)
  {
  }

  template <typename DeviceAdapter, typename... Args>
  void operator()(DeviceAdapter device,
                  vtkm::cont::DeviceAdapterId deviceId,
                  bool& status,
                  Args&&... args) const
  {
    if (device == deviceId)
    {
      VTKM_ASSERT(status == false);
      this->Functor(device, std::forward<Args>(args)...);
      status = true;
    }
  }

private:
  FunctorType Functor;
};

/// Finds the \c DeviceAdapterTag in \c DeviceList with id equal to deviceId
/// and executes the functor with the tag. Throws \c ErrorBadDevice if a valid
/// \c DeviceAdapterTag is not found.
///
template <typename DeviceList, typename Functor, typename... Args>
VTKM_CONT void FindDeviceAdapterTagAndCall(vtkm::cont::DeviceAdapterId deviceId,
                                           DeviceList devices,
                                           const Functor& functor,
                                           Args&&... args)
{
  bool status = false;
  ExecuteIfSameDeviceId<Functor> wrapped(functor);
  ForEachValidDevice(devices, wrapped, deviceId, status, std::forward<Args>(args)...);
  if (!status)
  {
    std::string msg = "Device with id " + std::to_string(deviceId.GetValue()) +
      " is either not in the list or is invalid";
    throw vtkm::cont::ErrorBadDevice(msg);
  }
}
}
}
} // vtkm::cont::internal

#endif // vtk_m_cont_internal_DeviceAdapterListHelpers_h
