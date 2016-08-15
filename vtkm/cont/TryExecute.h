//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_TryExecute_h
#define vtk_m_cont_TryExecute_h

#include <vtkm/cont/DeviceAdapterListTag.h>

namespace vtkm {
namespace cont {

namespace detail {

template<typename Functor, typename Device, bool DeviceAdapterValid>
struct TryExecuteRunIfValid;

template<typename Functor, typename Device>
struct TryExecuteRunIfValid<Functor, Device, false>
{
  VTKM_CONT_EXPORT
  static bool Run(Functor &) { return false; }
};

template<typename Functor, typename Device>
struct TryExecuteRunIfValid<Functor, typename Device, true>
{
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  VTKM_CONT_EXPORT
  static bool Run(Functor &functor)
  {
    try
    {
      return functor(Device());
    }
    catch (...)
    {
      return false;
    }
  }
};

template<typename FunctorType>
struct TryExecuteImpl
{
  // Warning, this is a reference. Make sure referenced object does not go out
  // of scope.
  FunctorType &Functor;

  bool Success;

  VTKM_CONT_EXPORT
  TryExecuteImpl(FunctorType &functor)
    : Functor(functor), Success(false) {  }

  template<typename Device>
  VTKM_CONT_EXPORT
  bool operator()(Device)
  {
    if (!this->Success)
    {
      typedef vtkm::cont::DeviceAdapterTraits<Device> DeviceTraits;

      this->Success =
          detail::TryExecuteRunIfValid<FunctorType,Device,DeviceTraits::Valid>
          ::Run(this->Functor);
    }

    return this->Success;
  }
};

} // namespace detail

/// \brief Try to execute a functor on a list of devices until one succeeds.
///
/// This function takes a functor an a list of devices. It then tries to run
/// the functor for each device (in the order given in the list) until the
/// execution succeeds.
///
/// The functor parentheses operator should take exactly one argument, which is
/// the \c DeviceAdapterTag to use. The functor should return a \c bool that is
/// \c true if the execution succeeds, \c false if it fails. If an exception is
/// thrown from the functor, then the execution is assumed to have failed.
///
/// This function returns \c true if the functor succeeded on a device,
/// \c false otherwise.
///
/// If no device list is specified, then \c VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG
/// is used.
///
template<typename Functor, typename DeviceList>
VTKM_CONT_EXPORT
bool TryExecute(const Functor &functor, DeviceList)
{
  detail::TryExecuteImpl<const Functor> internals(functor);
  vtkm::ListForEach(internals, DeviceList());
  return internals.Success;
}
template<typename Functor, typename DeviceList>
VTKM_CONT_EXPORT
bool TryExecute(Functor &functor, DeviceList)
{
  detail::TryExecuteImpl<Functor> internals(functor);
  vtkm::ListForEach(internals, DeviceList());
  return internals.Success;
}
template<typename Functor>
VTKM_CONT_EXPORT
bool TryExecute(const Functor &functor)
{
  return vtkm::cont::TryExecute(functor,
                                VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG());
}
template<typename Functor>
VTKM_CONT_EXPORT
bool TryExecute(Functor &functor)
{
  return vtkm::cont::TryExecute(functor,
                                VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG());
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_TryExecute_h
