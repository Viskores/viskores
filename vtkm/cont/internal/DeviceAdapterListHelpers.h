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
struct ExecuteIfValidDeviceTag
{

  template <typename DeviceAdapter>
  using EnableIfValid = std::enable_if<DeviceAdapter::IsEnabled>;

  template <typename DeviceAdapter>
  using EnableIfInvalid = std::enable_if<!DeviceAdapter::IsEnabled>;

  template <typename DeviceAdapter, typename Functor, typename... Args>
  typename EnableIfValid<DeviceAdapter>::type operator()(
    DeviceAdapter device,
    Functor&& f,
    const vtkm::cont::RuntimeDeviceTracker& tracker,
    Args&&... args) const
  {
    if (tracker.CanRunOn(device))
    {
      f(device, std::forward<Args>(args)...);
    }
  }

  // do not generate code for invalid devices
  template <typename DeviceAdapter, typename... Args>
  typename EnableIfInvalid<DeviceAdapter>::type operator()(DeviceAdapter, Args&&...) const
  {
  }
};

/// Execute the given functor on each valid device in \c DeviceList.
///
template <typename DeviceList, typename Functor, typename... Args>
VTKM_CONT void ForEachValidDevice(DeviceList devices, Functor&& functor, Args&&... args)
{
  auto tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker();
  vtkm::ListForEach(
    ExecuteIfValidDeviceTag{}, devices, functor, tracker, std::forward<Args>(args)...);
}
}
}
} // vtkm::cont::internal

#endif // vtk_m_cont_internal_DeviceAdapterListHelpers_h
