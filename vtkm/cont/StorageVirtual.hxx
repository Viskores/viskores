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

#include <vtkm/cont/StorageAny.hxx>
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
}

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
}

} // namespace vtkm::cont::

#endif
