//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_VirtualObjectHandle_h
#define vtk_m_cont_VirtualObjectHandle_h

#include <vtkm/cont/DeviceAdapterList.h>
#include <vtkm/cont/ExecutionAndControlObjectBase.h>
#include <vtkm/cont/internal/DeviceAdapterListHelpers.h>
#include <vtkm/cont/internal/VirtualObjectTransfer.h>

#include <array>
#include <type_traits>

namespace vtkm
{
namespace cont
{

namespace internal
{
struct CreateTransferInterface
{
  template <typename VirtualDerivedType, typename DeviceAdapter>
  VTKM_CONT inline void operator()(DeviceAdapter device,
                                   internal::TransferState* transfers,
                                   const VirtualDerivedType* virtualObject) const
  {
    using TransferImpl = TransferInterfaceImpl<VirtualDerivedType, DeviceAdapter>;
    auto i = static_cast<std::size_t>(device.GetValue());
    transfers->DeviceTransferState[i].reset(new TransferImpl(virtualObject));
  }
};
}

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
class VTKM_ALWAYS_EXPORT VirtualObjectHandle : public vtkm::cont::ExecutionAndControlObjectBase
{
  VTKM_STATIC_ASSERT_MSG((std::is_base_of<vtkm::VirtualObjectBase, VirtualBaseType>::value),
                         "All virtual objects must be subclass of vtkm::VirtualObjectBase.");

public:
  VTKM_CONT VirtualObjectHandle()
    : Internals(std::make_shared<internal::TransferState>())
  {
  }

  template <typename VirtualDerivedType,
            typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST>
  VTKM_CONT explicit VirtualObjectHandle(VirtualDerivedType* derived,
                                         bool acquireOwnership = true,
                                         DeviceAdapterList devices = DeviceAdapterList())
    : Internals(std::make_shared<internal::TransferState>())
  {
    this->Reset(derived, acquireOwnership, devices);
  }

  /// Get if in a valid state (a target is bound)
  VTKM_CONT bool GetValid() const { return this->Internals->HostPtr() != nullptr; }


  /// Get if this handle owns the control side target object
  VTKM_CONT bool OwnsObject() const { return this->Internals->WillReleaseHostPointer(); }

  /// Get the control side pointer to the virtual object
  VTKM_CONT VirtualBaseType* Get() const
  {
    return static_cast<VirtualBaseType*>(this->Internals->HostPtr());
  }

  /// Reset the underlying derived type object
  template <typename VirtualDerivedType,
            typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST>
  VTKM_CONT void Reset(VirtualDerivedType* derived,
                       bool acquireOwnership = true,
                       DeviceAdapterList devices = DeviceAdapterList())
  {
    VTKM_STATIC_ASSERT_MSG((std::is_base_of<VirtualBaseType, VirtualDerivedType>::value),
                           "Tried to bind a type that is not a subclass of the base class.");

    if (acquireOwnership)
    {
      auto deleter = [](void* p) { delete static_cast<VirtualBaseType*>(p); };
      this->Internals->UpdateHost(derived, deleter);
    }
    else
    {
      this->Internals->UpdateHost(derived, nullptr);
    }

    if (derived)
    {
      vtkm::cont::internal::ForEachValidDevice(
        devices, internal::CreateTransferInterface(), this->Internals.get(), derived);
    }
  }

  /// Release all host and execution side resources
  VTKM_CONT void ReleaseResources() { this->Internals->ReleaseResources(); }

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
    const bool validId = this->Internals->DeviceIdIsValid(deviceId);
    if (!validId)
    { //can't be reached since DeviceIdIsValid will through an exception
      //if deviceId is not valid
      return nullptr;
    }

    return static_cast<const VirtualBaseType*>(this->Internals->PrepareForExecution(deviceId));
  }

  /// Used as part of the \c ExecutionAndControlObjectBase interface. Returns the same pointer
  /// as \c Get.
  VTKM_CONT const VirtualBaseType* PrepareForControl() const { return this->Get(); }

private:
  std::shared_ptr<internal::TransferState> Internals;
};
}
} // vtkm::cont

#endif // vtk_m_cont_VirtualObjectHandle_h
