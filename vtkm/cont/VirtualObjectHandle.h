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
#ifndef vtk_m_cont_VirtualObjectHandle_h
#define vtk_m_cont_VirtualObjectHandle_h

#include <vtkm/cont/DeviceAdapterListTag.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/internal/DeviceAdapterListHelpers.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>
#include <vtkm/cont/internal/VirtualObjectTransfer.h>

#include <array>
#include <type_traits>

namespace vtkm
{
namespace cont
{

/// \brief Implements VTK-m's execution side <em> Virtual Methods </em> functionality.
///
/// The template parameter \c VirtualBaseType is the base class that acts as the
/// interface. This base clase must inherit from \c vtkm::VirtualObjectBase. See the
/// documentation of that class to see the other requirements.
///
/// A derived object can be bound to the handle either during construction or using the \c Reset
/// function on previously constructed handles. These function accept a control side pointer to
/// the derived class object, a boolean flag stating if the handle should acquire ownership of
/// the object (i.e. manage the lifetime), and a type-list of device adapter tags where the object
/// is expected to be used.
///
/// To get an execution side pointer call the \c PrepareForExecution function. The device adapter
/// passed to this function should be one of the device in the list passed during the set up of
/// the handle.
///
///
/// \sa vtkm::VirtualObjectBase
///
template <typename VirtualBaseType>
class VTKM_ALWAYS_EXPORT VirtualObjectHandle
{
  VTKM_STATIC_ASSERT_MSG((std::is_base_of<vtkm::VirtualObjectBase, VirtualBaseType>::value),
                         "All virtual objects must be subclass of vtkm::VirtualObjectBase.");

public:
  VTKM_CONT VirtualObjectHandle()
    : Internals(new InternalStruct)
  {
  }

  template <typename VirtualDerivedType,
            typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG>
  VTKM_CONT explicit VirtualObjectHandle(VirtualDerivedType* derived,
                                         bool acquireOwnership = true,
                                         DeviceAdapterList devices = DeviceAdapterList())
    : Internals(new InternalStruct)
  {
    this->Reset(derived, acquireOwnership, devices);
  }

  /// Get if in a valid state (a target is bound)
  VTKM_CONT bool GetValid() const { return this->Internals->VirtualObject != nullptr; }

  /// Get if this handle owns the control side target object
  VTKM_CONT bool OwnsObject() const { return this->Internals->Owner; }

  /// Get the control side pointer to the virtual object
  VTKM_CONT VirtualBaseType* Get() const { return this->Internals->VirtualObject; }

  /// Reset the underlying derived type object
  template <typename VirtualDerivedType,
            typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG>
  VTKM_CONT void Reset(VirtualDerivedType* derived,
                       bool acquireOwnership = true,
                       DeviceAdapterList devices = DeviceAdapterList())
  {
    this->Reset();
    if (derived)
    {
      VTKM_STATIC_ASSERT_MSG((std::is_base_of<VirtualBaseType, VirtualDerivedType>::value),
                             "Tried to bind a type that is not a subclass of the base class.");

      this->Internals->VirtualObject = derived;
      this->Internals->Owner = acquireOwnership;
      vtkm::cont::internal::ForEachValidDevice(devices,
                                               CreateTransferInterface<VirtualDerivedType>(),
                                               this->Internals->Transfers.data(),
                                               derived);
    }
  }

  void Reset() { this->Internals->Reset(); }

  /// Release all the execution side resources
  VTKM_CONT void ReleaseExecutionResources() { this->Internals->ReleaseExecutionResources(); }

  /// Get a valid \c VirtualBaseType* with the current control side state for \c deviceId.
  /// VirtualObjectHandle and the returned pointer are analogous to ArrayHandle and Portal
  /// The returned pointer will be invalidated if:
  /// 1. A new pointer is requested for a different deviceId
  /// 2. VirtualObjectHandle is destroyed
  /// 3. Reset or ReleaseResources is called
  ///
  VTKM_CONT const VirtualBaseType* PrepareForExecution(vtkm::cont::DeviceAdapterId deviceId) const
  {
    if (!this->GetValid())
    {
      throw vtkm::cont::ErrorBadValue("No target object bound");
    }

    if (!this->Internals->Current || this->Internals->Current->GetDeviceId() != deviceId)
    {
      if (!this->Internals->Transfers[static_cast<std::size_t>(deviceId.GetValue())])
      {
        std::string msg = "VTK-m was asked to transfer an object for execution on DeviceAdapter " +
          std::to_string(deviceId.GetValue()) +
          ". It can't as this VirtualObjectHandle was not constructed/bound with this "
          "DeviceAdapter in the list of valid DeviceAdapters.";
        throw vtkm::cont::ErrorBadType(msg);
      }

      if (this->Internals->Current)
      {
        this->Internals->Current->ReleaseResources();
      }
      this->Internals->Current =
        this->Internals->Transfers[static_cast<std::size_t>(deviceId.GetValue())].get();
    }

    return this->Internals->Current->PrepareForExecution();
  }


private:
  class TransferInterface
  {
  public:
    VTKM_CONT virtual ~TransferInterface() = default;

    VTKM_CONT virtual vtkm::cont::DeviceAdapterId GetDeviceId() const = 0;
    VTKM_CONT virtual const VirtualBaseType* PrepareForExecution() = 0;
    VTKM_CONT virtual void ReleaseResources() = 0;
  };

  template <typename VirtualDerivedType, typename DeviceAdapter>
  class TransferInterfaceImpl : public TransferInterface
  {
  public:
    VTKM_CONT TransferInterfaceImpl(const VirtualDerivedType* virtualObject)
      : LastModifiedCount(-1)
      , VirtualObject(virtualObject)
      , Transfer(virtualObject)
    {
    }

    VTKM_CONT vtkm::cont::DeviceAdapterId GetDeviceId() const override { return DeviceAdapter(); }

    VTKM_CONT const VirtualBaseType* PrepareForExecution() override
    {
      vtkm::Id modifiedCount = this->VirtualObject->GetModifiedCount();
      bool updateData = (this->LastModifiedCount != modifiedCount);
      const VirtualBaseType* executionObject = this->Transfer.PrepareForExecution(updateData);
      this->LastModifiedCount = modifiedCount;
      return executionObject;
    }

    VTKM_CONT void ReleaseResources() override { this->Transfer.ReleaseResources(); }

  private:
    vtkm::Id LastModifiedCount;
    const VirtualDerivedType* VirtualObject;
    vtkm::cont::internal::VirtualObjectTransfer<VirtualDerivedType, DeviceAdapter> Transfer;
  };

  template <typename VirtualDerivedType>
  struct CreateTransferInterface
  {
    template <typename DeviceAdapter>
    VTKM_CONT void operator()(DeviceAdapter device,
                              std::unique_ptr<TransferInterface>* transfers,
                              const VirtualDerivedType* virtualObject) const
    {
      using DeviceInfo = vtkm::cont::DeviceAdapterTraits<DeviceAdapter>;
      if (!device.IsValueValid())
      {

        std::string msg =
          "VTK-m is unable to construct a VirtualObjectHandle for execution on DeviceAdapter" +
          DeviceInfo::GetName() + "[id=" + std::to_string(device.GetValue()) +
          "]. This is generally caused by either asking for execution on a DeviceAdapter that "
          "wasn't compiled into VTK-m. In the case of CUDA it can also be caused by accidentally "
          "compiling source files as C++ files instead of CUDA.";
        throw vtkm::cont::ErrorBadType(msg);
      }
      using TransferImpl = TransferInterfaceImpl<VirtualDerivedType, DeviceAdapter>;
      transfers[device.GetValue()].reset(new TransferImpl(virtualObject));
    }
  };

  struct InternalStruct
  {
    VirtualBaseType* VirtualObject = nullptr;
    bool Owner = false;
    std::array<std::unique_ptr<TransferInterface>, VTKM_MAX_DEVICE_ADAPTER_ID> Transfers;
    TransferInterface* Current = nullptr;

    VTKM_CONT void ReleaseExecutionResources()
    {
      if (this->Current)
      {
        this->Current->ReleaseResources();
        this->Current = nullptr;
      }
    }

    VTKM_CONT void Reset()
    {
      this->ReleaseExecutionResources();
      for (auto& transfer : this->Transfers)
      {
        transfer.reset(nullptr);
      }
      if (this->Owner)
      {
        delete this->VirtualObject;
      }
      this->VirtualObject = nullptr;
      this->Owner = false;
    }

    VTKM_CONT ~InternalStruct() { this->Reset(); }
  };

  std::shared_ptr<InternalStruct> Internals;
};
}
} // vtkm::cont

#endif // vtk_m_cont_VirtualObjectHandle_h
