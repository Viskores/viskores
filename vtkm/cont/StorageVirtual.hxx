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
#ifndef vtk_m_cont_StorageVirtual_hxx
#define vtk_m_cont_StorageVirtual_hxx

#include <vtkm/cont/StorageVirtual.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/internal/TransferInfo.h>

#include <vtkm/cont/internal/VirtualObjectTransferShareWithControl.h>

namespace vtkm
{
namespace cont
{
namespace detail
{
template <typename DerivedPortal>
struct TransferToDevice
{
  template <typename DeviceAdapterTag, typename Payload, typename... Args>
  inline bool operator()(DeviceAdapterTag devId, Payload&& payload, Args&&... args) const
  {
    using TransferType = cont::internal::VirtualObjectTransfer<DerivedPortal, DeviceAdapterTag>;
    using shared_memory_transfer =
      std::is_base_of<vtkm::cont::internal::VirtualObjectTransferShareWithControl<DerivedPortal>,
                      TransferType>;

    return this->Transfer(
      devId, shared_memory_transfer{}, std::forward<Payload>(payload), std::forward<Args>(args)...);
  }

  template <typename DeviceAdapterTag, typename Payload, typename... Args>
  inline bool Transfer(DeviceAdapterTag devId,
                       std::true_type,
                       Payload&& payload,
                       Args&&... args) const
  { //shared memory transfer so we just need
    auto smp_ptr = new DerivedPortal(std::forward<Args>(args)...);
    auto host = std::unique_ptr<DerivedPortal>(smp_ptr);
    payload.updateDevice(devId, std::move(host), smp_ptr, nullptr);

    return true;
  }

  template <typename DeviceAdapterTag, typename Payload, typename... Args>
  inline bool Transfer(DeviceAdapterTag devId,
                       std::false_type,
                       Payload&& payload,
                       Args&&... args) const
  { //separate memory transfer
    //construct all new transfer payload
    using TransferType = cont::internal::VirtualObjectTransfer<DerivedPortal, DeviceAdapterTag>;

    auto host = std::unique_ptr<DerivedPortal>(new DerivedPortal(std::forward<Args>(args)...));
    auto transfer = std::make_shared<TransferType>(host.get());
    auto device = transfer->PrepareForExecution(true);

    payload.updateDevice(devId, std::move(host), device, std::static_pointer_cast<void>(transfer));

    return true;
  }
};
} // namespace detail

template <typename DerivedPortal, typename... Args>
inline void make_transferToDevice(vtkm::cont::DeviceAdapterId devId, Args&&... args)
{
  vtkm::cont::TryExecuteOnDevice(
    devId, detail::TransferToDevice<DerivedPortal>{}, std::forward<Args>(args)...);
}

template <typename DerivedPortal, typename Payload, typename... Args>
inline void make_hostPortal(Payload&& payload, Args&&... args)
{
  auto host = std::unique_ptr<DerivedPortal>(new DerivedPortal(std::forward<Args>(args)...));
  payload.updateHost(std::move(host));
}

namespace internal
{
namespace detail
{

VTKM_CONT
template <typename T, typename S>
StorageVirtualImpl<T, S>::StorageVirtualImpl(const vtkm::cont::ArrayHandle<T, S>& ah)
  : vtkm::cont::internal::detail::StorageVirtual()
  , Handle(ah)
{
}

VTKM_CONT
template <typename T, typename S>
StorageVirtualImpl<T, S>::StorageVirtualImpl(vtkm::cont::ArrayHandle<T, S>&& ah) noexcept
  : vtkm::cont::internal::detail::StorageVirtual(),
    Handle(std::move(ah))
{
}

/// release execution side resources
template <typename T, typename S>
void StorageVirtualImpl<T, S>::ReleaseResourcesExecution()
{
  this->DropExecutionPortal();
  this->Handle.ReleaseResourcesExecution();
}

/// release control side resources
template <typename T, typename S>
void StorageVirtualImpl<T, S>::ReleaseResources()
{
  this->DropAllPortals();
  this->Handle.ReleaseResources();
}

template <typename T, typename S>
void StorageVirtualImpl<T, S>::Allocate(vtkm::Id numberOfValues)
{
  this->DropAllPortals();
  this->Handle.Allocate(numberOfValues);
}

template <typename T, typename S>
void StorageVirtualImpl<T, S>::Shrink(vtkm::Id numberOfValues)
{
  this->DropAllPortals();
  this->Handle.Shrink(numberOfValues);
}

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
                  vtkm::cont::internal::detail::StorageVirtual::OutputMode mode) const
  {
    using ACCESS_MODE = vtkm::cont::internal::detail::StorageVirtual::OutputMode;
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

template <typename T, typename S>
void StorageVirtualImpl<T, S>::ControlPortalForInput(
  vtkm::cont::internal::TransferInfoArray& payload) const
{
  auto portal = this->Handle.GetPortalConstControl();
  using DerivedPortal = vtkm::ArrayPortalWrapper<decltype(portal)>;
  vtkm::cont::make_hostPortal<DerivedPortal>(payload, portal);
}

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

template <typename T, typename S>
void StorageVirtualImpl<T, S>::ControlPortalForOutput(
  vtkm::cont::internal::TransferInfoArray& payload)
{
  using HT = vtkm::cont::ArrayHandle<T, S>;
  constexpr auto isWriteable = typename vtkm::cont::internal::IsWriteableArrayHandle<HT>::type{};

  detail::make_writableHostPortal(isWriteable, payload, this->Handle);
}

template <typename T, typename S>
void StorageVirtualImpl<T, S>::TransferPortalForInput(
  vtkm::cont::internal::TransferInfoArray& payload,
  vtkm::cont::DeviceAdapterId devId) const
{
  vtkm::cont::TryExecuteOnDevice(devId, detail::PortalWrapperToDevice(), this->Handle, payload);
}


template <typename T, typename S>
void StorageVirtualImpl<T, S>::TransferPortalForOutput(
  vtkm::cont::internal::TransferInfoArray& payload,
  vtkm::cont::internal::detail::StorageVirtual::OutputMode mode,
  vtkm::Id numberOfValues,
  vtkm::cont::DeviceAdapterId devId)
{
  vtkm::cont::TryExecuteOnDevice(
    devId, detail::PortalWrapperToDevice(), this->Handle, numberOfValues, payload, mode);
}
} // namespace detail

template <typename T>
void Storage<T, vtkm::cont::StorageTagVirtual>::Allocate(vtkm::Id numberOfValues)
{
  if (this->VirtualStorage)
  {
    this->VirtualStorage->Allocate(numberOfValues);
  }
  else if (numberOfValues != 0)
  {
    throw vtkm::cont::ErrorBadAllocation("Attempted to allocate memory in a virtual array that "
                                         "does not have an underlying concrete array.");
  }
  else
  {
    // Allocating a non-existing array to 0 is OK.
  }
}

template <typename T>
void Storage<T, vtkm::cont::StorageTagVirtual>::Shrink(vtkm::Id numberOfValues)
{
  if (this->VirtualStorage)
  {
    this->VirtualStorage->Shrink(numberOfValues);
  }
  else if (numberOfValues != 0)
  {
    throw vtkm::cont::ErrorBadAllocation(
      "Attempted to shrink a virtual array that does not have an underlying concrete array.");
  }
  else
  {
    // Shrinking a non-existing array to 0 is OK.
  }
}

template <typename T>
void Storage<T, vtkm::cont::StorageTagVirtual>::ReleaseResources()
{
  if (this->VirtualStorage)
  {
    this->VirtualStorage->ReleaseResources();
  }
  else
  {
    // No concrete array, nothing allocated, nothing to do.
  }
}

template <typename T>
Storage<T, vtkm::cont::StorageTagVirtual> Storage<T, vtkm::cont::StorageTagVirtual>::NewInstance()
  const
{
  if (this->GetStorageVirtual())
  {
    return Storage<T, vtkm::cont::StorageTagVirtual>(this->GetStorageVirtual()->NewInstance());
  }
  else
  {
    return Storage<T, vtkm::cont::StorageTagVirtual>();
  }
}
}
}
} // namespace vtkm::cont::internal

#endif
