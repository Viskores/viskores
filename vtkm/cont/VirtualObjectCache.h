//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_VirtualObjectCache_h
#define vtk_m_cont_VirtualObjectCache_h

#include <vtkm/cont/DeviceAdapterListTag.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>
#include <vtkm/cont/internal/VirtualObjectTransfer.h>

#include <array>
#include <type_traits>

#define VTKM_MAX_DEVICE_ADAPTER_ID 8

namespace vtkm
{
namespace cont
{

/// \brief Implements VTK-m's execution side <em> Virtual Methods </em>
/// functionality.
///
/// The template parameter \c VirtualObject is the class that acts as the
/// interface. Following is a method for implementing such classes:
/// 1. Create a <tt>const void*</tt> member variable that will hold a
///    reference to the target class.
/// 2. For each virtual-like method:
///    a. Create a typedef for a function with the same signature,
///       except for an extra <tt>const void*</tt> argument.
///    b. Create a function pointer member variable with the type of the
///       associated typedef from 2a.
///    c. Create an implementation of the method, that calls the associated
///       function pointer with the void pointer to the target class as one of
///       the arguments.
/// 3. Create the following template function:
///    \code{.cpp}
///       template<typename TargetClass>
///       VTKM_EXEC void Bind(const TargetClass *deviceTarget);
///    \endcode
///    This function should set the void pointer from 1 to \c deviceTarget,
///    and assign the function pointers from 2b to functions that cast their
///    first argument to <tt>const TargetClass*</tt> and call the corresponding
///    member function on it.
///
/// Both \c VirtualObject and target class objects should be bitwise copyable.
///
/// \sa vtkm::exec::ImplicitFunction, vtkm::cont::ImplicitFunction
///
template <typename VirtualObject>
class VirtualObjectCache
{
public:
  VirtualObjectCache()
    : Target(nullptr)
    , CurrentDevice(VTKM_DEVICE_ADAPTER_UNDEFINED)
    , DeviceState(nullptr)
    , RefreshFlag(false)
  {
  }

  ~VirtualObjectCache() { this->Reset(); }

  VirtualObjectCache(const VirtualObjectCache& other)
    : Target(other.Target)
    , Transfers(other.Transfers)
    , CurrentDevice(VTKM_DEVICE_ADAPTER_UNDEFINED)
    , DeviceState(nullptr)
    , RefreshFlag(false)
  {
  }

  VirtualObjectCache& operator=(const VirtualObjectCache& other)
  {
    if (this != &other)
    {
      this->Target = other.Target;
      this->Transfers = other.Transfers;
      this->CurrentDevice = VTKM_DEVICE_ADAPTER_UNDEFINED;
      this->DeviceState = nullptr;
      this->RefreshFlag = false;
      this->Object = VirtualObject();
    }
    return *this;
  }

  VirtualObjectCache(VirtualObjectCache&& other)
    : Target(other.Target)
    , Transfers(other.Transfers)
    , CurrentDevice(other.CurrentDevice)
    , DeviceState(other.DeviceState)
    , RefreshFlag(other.RefreshFlag)
    , Object(other.Object)
  {
    other.CurrentDevice = VTKM_DEVICE_ADAPTER_UNDEFINED;
    other.DeviceState = nullptr;
  }

  VirtualObjectCache& operator=(VirtualObjectCache&& other)
  {
    if (this != &other)
    {
      this->Target = other.Target;
      this->Transfers = std::move(other.Transfers);
      this->CurrentDevice = other.CurrentDevice;
      this->DeviceState = other.DeviceState;
      this->RefreshFlag = other.RefreshFlag;
      this->Object = std::move(other.Object);

      other.CurrentDevice = VTKM_DEVICE_ADAPTER_UNDEFINED;
      other.DeviceState = nullptr;
    }
    return *this;
  }

  /// Reset to the default constructed state
  void Reset()
  {
    this->ReleaseResources();

    this->Target = nullptr;
    this->Transfers.fill(TransferInterface());
  }

  void ReleaseResources()
  {
    if (this->CurrentDevice > 0)
    {
      this->GetCurrentTransfer().Cleanup(this->DeviceState);
      this->CurrentDevice = VTKM_DEVICE_ADAPTER_UNDEFINED;
      this->DeviceState = nullptr;
      this->RefreshFlag = false;
      this->Object = VirtualObject();
    }
  }

  /// Get if in a valid state (a target is bound)
  bool GetValid() const { return this->Target != nullptr; }

  // Set/Get if the cached virtual object should be refreshed to the current
  // state of the target
  void SetRefreshFlag(bool value) { this->RefreshFlag = value; }
  bool GetRefreshFlag() const { return this->RefreshFlag; }

  /// Bind to \c target. The lifetime of target is expected to be managed
  /// externally, and should be valid for as long as it is bound.
  /// Also accepts a list-tag of device adapters where the virtual
  /// object may be used (default = \c VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG).
  ///
  template <typename TargetClass, typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG>
  void Bind(const TargetClass* target, DeviceAdapterList devices = DeviceAdapterList())
  {
    this->Reset();

    this->Target = target;
    vtkm::ListForEach(CreateTransferInterface<TargetClass>(this->Transfers.data()), devices);
  }

  /// Get a \c VirtualObject for \c DeviceAdapter.
  /// VirtualObjectCache and VirtualObject are analogous to ArrayHandle and Portal
  /// The returned VirtualObject will be invalidated if:
  /// 1. A new VirtualObject is requested for a different DeviceAdapter
  /// 2. VirtualObjectCache is destroyed
  /// 3. Reset or ReleaseResources is called
  ///
  template <typename DeviceAdapter>
  VirtualObject GetVirtualObject(DeviceAdapter)
  {
    using DeviceInfo = vtkm::cont::DeviceAdapterTraits<DeviceAdapter>;

    if (!this->GetValid())
    {
      throw vtkm::cont::ErrorBadValue("No target object bound");
    }

    vtkm::cont::DeviceAdapterId deviceId = DeviceInfo::GetId();
    if (deviceId < 0 || deviceId >= VTKM_MAX_DEVICE_ADAPTER_ID)
    {
      std::string msg = "Device '" + DeviceInfo::GetName() + "' has invalid ID of " +
        std::to_string(deviceId) + "(VTKM_MAX_DEVICE_ADAPTER_ID = " +
        std::to_string(VTKM_MAX_DEVICE_ADAPTER_ID) + ")";
      throw vtkm::cont::ErrorBadType(msg);
    }

    if (this->CurrentDevice != deviceId)
    {
      this->ReleaseResources();

      std::size_t idx = static_cast<std::size_t>(deviceId);
      TransferInterface& transfer = this->Transfers[idx];
      if (!TransferInterfaceValid(transfer))
      {
        std::string msg = DeviceInfo::GetName() + " was not in the list specified in Bind";
        throw vtkm::cont::ErrorBadType(msg);
      }
      this->CurrentDevice = deviceId;
      this->DeviceState = transfer.Create(this->Object, this->Target);
    }
    else if (this->RefreshFlag)
    {
      this->GetCurrentTransfer().Update(this->DeviceState, this->Target);
    }

    this->RefreshFlag = false;
    return this->Object;
  }

private:
  struct TransferInterface
  {
    using CreateSig = void*(VirtualObject&, const void*);
    using UpdateSig = void(void*, const void*);
    using CleanupSig = void(void*);

    CreateSig* Create = nullptr;
    UpdateSig* Update = nullptr;
    CleanupSig* Cleanup = nullptr;
  };

  static bool TransferInterfaceValid(const TransferInterface& t) { return t.Create != nullptr; }

  TransferInterface& GetCurrentTransfer()
  {
    return this->Transfers[static_cast<std::size_t>(this->CurrentDevice)];
  }

  template <typename TargetClass>
  class CreateTransferInterface
  {
  private:
    template <typename DeviceAdapter>
    using EnableIfValid = std::enable_if<vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::Valid>;

    template <typename DeviceAdapter>
    using EnableIfInvalid = std::enable_if<!vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::Valid>;

  public:
    CreateTransferInterface(TransferInterface* transfers)
      : Transfers(transfers)
    {
    }

    // Use SFINAE to create entries for valid device adapters only
    template <typename DeviceAdapter>
    typename EnableIfValid<DeviceAdapter>::type operator()(DeviceAdapter) const
    {
      using DeviceInfo = vtkm::cont::DeviceAdapterTraits<DeviceAdapter>;

      if (DeviceInfo::GetId() >= 0 && DeviceInfo::GetId() < VTKM_MAX_DEVICE_ADAPTER_ID)
      {
        using TransferImpl =
          internal::VirtualObjectTransfer<VirtualObject, TargetClass, DeviceAdapter>;

        std::size_t id = static_cast<std::size_t>(DeviceInfo::GetId());
        TransferInterface& transfer = this->Transfers[id];

        transfer.Create = &TransferImpl::Create;
        transfer.Update = &TransferImpl::Update;
        transfer.Cleanup = &TransferImpl::Cleanup;
      }
      else
      {
        std::string msg = "Device '" + DeviceInfo::GetName() + "' has invalid ID of " +
          std::to_string(DeviceInfo::GetId()) + "(VTKM_MAX_DEVICE_ADAPTER_ID = " +
          std::to_string(VTKM_MAX_DEVICE_ADAPTER_ID) + ")";
        throw vtkm::cont::ErrorBadType(msg);
      }
    }

    template <typename DeviceAdapter>
    typename EnableIfInvalid<DeviceAdapter>::type operator()(DeviceAdapter) const
    {
    }

  private:
    TransferInterface* Transfers;
  };

  const void* Target;
  std::array<TransferInterface, VTKM_MAX_DEVICE_ADAPTER_ID> Transfers;

  vtkm::cont::DeviceAdapterId CurrentDevice;
  void* DeviceState;
  bool RefreshFlag;
  VirtualObject Object;
};
}
} // vtkm::cont

#undef VTKM_MAX_DEVICE_ADAPTER_ID

#endif // vtk_m_cont_VirtualObjectCache_h
