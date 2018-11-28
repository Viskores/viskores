//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleAny_hxx
#define vtk_m_cont_ArrayHandleAny_hxx

#include <vtkm/cont/ArrayHandleAny.h>
#include <vtkm/cont/ArrayHandleVirtual.hxx>

namespace vtkm
{
namespace cont
{

VTKM_CONT
template <typename T, typename S>
StorageAny<T, S>::StorageAny(const vtkm::cont::ArrayHandle<T, S>& ah)
  : vtkm::cont::StorageVirtual()
  , Handle(ah)
{
}

/// release execution side resources
template <typename T, typename S>
void StorageAny<T, S>::ReleaseResourcesExecution()
{
  vtkm::cont::StorageVirtual::ReleaseResourcesExecution();
  this->Handle.ReleaseResourcesExecution();
}

/// release control side resources
template <typename T, typename S>
void StorageAny<T, S>::ReleaseResources()
{
  vtkm::cont::StorageVirtual::ReleaseResources();
  this->Handle.ReleaseResources();
}

namespace detail
{
struct PortalWrapperToDevice
{
  template <typename DeviceAdapterTag, typename Handle>
  bool operator()(DeviceAdapterTag device,
                  Handle&& handle,
                  vtkm::cont::internal::TransferInfoArray& payload) const
  {
    auto portal = handle.PrepareForInput(device);
    using DerivedPortal = vtkm::ArrayPortalWrapper<decltype(portal)>;
    vtkm::cont::detail::TransferToDevice<DerivedPortal> transfer;
    return transfer(device, payload, portal);
  }
  template <typename DeviceAdapterTag, typename Handle>
  bool operator()(DeviceAdapterTag device,
                  Handle&& handle,
                  vtkm::Id numberOfValues,
                  vtkm::cont::internal::TransferInfoArray& payload,
                  vtkm::cont::StorageVirtual::OutputMode mode) const
  {
    using ACCESS_MODE = vtkm::cont::StorageVirtual::OutputMode;
    if (mode == ACCESS_MODE::WRITE)
    {
      auto portal = handle.PrepareForOutput(numberOfValues, device);
      using DerivedPortal = vtkm::ArrayPortalWrapper<decltype(portal)>;
      vtkm::cont::detail::TransferToDevice<DerivedPortal> transfer;
      return transfer(device, payload, portal);
    }
    else
    {
      auto portal = handle.PrepareForInPlace(device);
      using DerivedPortal = vtkm::ArrayPortalWrapper<decltype(portal)>;
      vtkm::cont::detail::TransferToDevice<DerivedPortal> transfer;
      return transfer(device, payload, portal);
    }
  }
};
}

template <typename T, typename S>
void StorageAny<T, S>::ControlPortalForInput(vtkm::cont::internal::TransferInfoArray& payload) const
{
  auto portal = this->Handle.GetPortalConstControl();
  using DerivedPortal = vtkm::ArrayPortalWrapper<decltype(portal)>;
  vtkm::cont::make_hostPortal<DerivedPortal>(payload, portal);
}

namespace detail
{
template <typename HandleType>
void make_writableHostPortal(std::true_type,
                             vtkm::cont::internal::TransferInfoArray& payload,
                             HandleType& handle)
{
  auto portal = handle.GetPortalControl();
  using DerivedPortal = vtkm::ArrayPortalWrapper<decltype(portal)>;
  vtkm::cont::make_hostPortal<DerivedPortal>(payload, portal);
}
template <typename HandleType>
void make_writableHostPortal(std::false_type,
                             vtkm::cont::internal::TransferInfoArray& payload,
                             HandleType&)
{
  payload.updateHost(nullptr);
  throw vtkm::cont::ErrorBadValue(
    "ArrayHandleAny was bound to an ArrayHandle that doesn't support output.");
}
}

template <typename T, typename S>
void StorageAny<T, S>::ControlPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload)
{
  using HT = vtkm::cont::ArrayHandle<T, S>;
  constexpr auto isWriteable = typename vtkm::cont::internal::IsWriteableArrayHandle<HT>::type{};

  detail::make_writableHostPortal(isWriteable, payload, this->Handle);
}

template <typename T, typename S>
void StorageAny<T, S>::TransferPortalForInput(vtkm::cont::internal::TransferInfoArray& payload,
                                              vtkm::cont::DeviceAdapterId devId) const
{
  vtkm::cont::TryExecuteOnDevice(devId, detail::PortalWrapperToDevice(), this->Handle, payload);
}


template <typename T, typename S>
void StorageAny<T, S>::TransferPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload,
                                               vtkm::cont::StorageVirtual::OutputMode mode,
                                               vtkm::Id numberOfValues,
                                               vtkm::cont::DeviceAdapterId devId)
{
  vtkm::cont::TryExecuteOnDevice(
    devId, detail::PortalWrapperToDevice(), this->Handle, numberOfValues, payload, mode);
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleAny_hxx
