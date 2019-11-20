//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_internal_ArrayHandleBasicImpl_hxx
#define vtk_m_cont_internal_ArrayHandleBasicImpl_hxx

#include <vtkm/cont/internal/ArrayHandleBasicImpl.h>

namespace vtkm
{
namespace cont
{
template <typename T>
ArrayHandle<T, StorageTagBasic>::ArrayHandle()
  : Internals(new internal::ArrayHandleImpl(T{}))
{
}

template <typename T>
ArrayHandle<T, StorageTagBasic>::ArrayHandle(const Thisclass& src)
  : Internals(src.Internals)
{
}

template <typename T>
ArrayHandle<T, StorageTagBasic>::ArrayHandle(Thisclass&& src) noexcept
  : Internals(std::move(src.Internals))
{
}

template <typename T>
ArrayHandle<T, StorageTagBasic>::ArrayHandle(const StorageType& storage) noexcept
  : Internals(new internal::ArrayHandleImpl(storage))
{
}

template <typename T>
ArrayHandle<T, StorageTagBasic>::ArrayHandle(StorageType&& storage) noexcept
  : Internals(new internal::ArrayHandleImpl(std::move(storage)))
{
}

template <typename T>
ArrayHandle<T, StorageTagBasic>::~ArrayHandle()
{
}

template <typename T>
ArrayHandle<T, StorageTagBasic>& ArrayHandle<T, StorageTagBasic>::operator=(const Thisclass& src)
{
  this->Internals = src.Internals;
  return *this;
}

template <typename T>
ArrayHandle<T, StorageTagBasic>& ArrayHandle<T, StorageTagBasic>::operator=(
  Thisclass&& src) noexcept
{
  this->Internals = std::move(src.Internals);
  return *this;
}

template <typename T>
bool ArrayHandle<T, StorageTagBasic>::operator==(const Thisclass& rhs) const
{
  return this->Internals == rhs.Internals;
}

template <typename T>
bool ArrayHandle<T, StorageTagBasic>::operator!=(const Thisclass& rhs) const
{
  return this->Internals != rhs.Internals;
}

template <typename T>
template <typename VT, typename ST>
VTKM_CONT bool ArrayHandle<T, StorageTagBasic>::operator==(const ArrayHandle<VT, ST>&) const
{
  return false; // different valuetype and/or storage
}

template <typename T>
template <typename VT, typename ST>
VTKM_CONT bool ArrayHandle<T, StorageTagBasic>::operator!=(const ArrayHandle<VT, ST>&) const
{
  return true; // different valuetype and/or storage
}

template <typename T>
typename ArrayHandle<T, StorageTagBasic>::StorageType& ArrayHandle<T, StorageTagBasic>::GetStorage()
{
  LockType lock = this->GetLock();
  this->SyncControlArray(lock);
  this->Internals->CheckControlArrayValid(lock);
  //CheckControlArrayValid will throw an exception if this->Internals->ControlArrayValid
  //is not valid

  return *(static_cast<StorageType*>(this->Internals->Internals->GetControlArray(lock)));
}

template <typename T>
const typename ArrayHandle<T, StorageTagBasic>::StorageType&
ArrayHandle<T, StorageTagBasic>::GetStorage() const
{
  LockType lock = this->GetLock();
  this->SyncControlArray(lock);
  this->Internals->CheckControlArrayValid(lock);
  //CheckControlArrayValid will throw an exception if this->Internals->ControlArrayValid
  //is not valid

  return *(static_cast<const StorageType*>(this->Internals->Internals->GetControlArray(lock)));
}

template <typename T>
typename ArrayHandle<T, StorageTagBasic>::PortalControl
ArrayHandle<T, StorageTagBasic>::GetPortalControl()
{
  LockType lock = this->GetLock();
  this->SyncControlArray(lock);
  this->Internals->CheckControlArrayValid(lock);
  //CheckControlArrayValid will throw an exception if this->Internals->ControlArrayValid
  //is not valid


  // If the user writes into the iterator we return, then the execution
  // array will become invalid. Play it safe and release the execution
  // resources. (Use the const version to preserve the execution array.)
  this->ReleaseResourcesExecutionInternal(lock);
  StorageType* privStorage =
    static_cast<StorageType*>(this->Internals->Internals->GetControlArray(lock));
  return privStorage->GetPortal();
}


template <typename T>
typename ArrayHandle<T, StorageTagBasic>::PortalConstControl
ArrayHandle<T, StorageTagBasic>::GetPortalConstControl() const
{
  LockType lock = this->GetLock();
  this->SyncControlArray(lock);
  this->Internals->CheckControlArrayValid(lock);
  //CheckControlArrayValid will throw an exception if this->Internals->ControlArrayValid
  //is not valid

  StorageType* privStorage =
    static_cast<StorageType*>(this->Internals->Internals->GetControlArray(lock));
  return privStorage->GetPortalConst();
}

template <typename T>
vtkm::Id ArrayHandle<T, StorageTagBasic>::GetNumberOfValues() const
{
  LockType lock = this->GetLock();
  return this->Internals->GetNumberOfValues(lock, sizeof(T));
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::Allocate(vtkm::Id numberOfValues)
{
  LockType lock = this->GetLock();
  this->Internals->Allocate(lock, numberOfValues, sizeof(T));
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::Shrink(vtkm::Id numberOfValues)
{
  LockType lock = this->GetLock();
  this->Internals->Shrink(lock, numberOfValues, sizeof(T));
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::ReleaseResourcesExecution()
{
  LockType lock = this->GetLock();
  // Save any data in the execution environment by making sure it is synced
  // with the control environment.
  this->SyncControlArray(lock);
  this->Internals->ReleaseResourcesExecutionInternal(lock);
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::ReleaseResources()
{
  LockType lock = this->GetLock();
  this->Internals->ReleaseResources(lock);
}

template <typename T>
template <typename DeviceAdapterTag>
typename ArrayHandle<T, StorageTagBasic>::template ExecutionTypes<DeviceAdapterTag>::PortalConst
ArrayHandle<T, StorageTagBasic>::PrepareForInput(DeviceAdapterTag device) const
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);
  LockType lock = this->GetLock();
  this->PrepareForDevice(lock, device);

  this->Internals->PrepareForInput(lock, sizeof(T));
  return PortalFactory<DeviceAdapterTag>::CreatePortalConst(
    static_cast<T*>(this->Internals->Internals->GetExecutionArray(lock)),
    static_cast<T*>(this->Internals->Internals->GetExecutionArrayEnd(lock)));
}

template <typename T>
template <typename DeviceAdapterTag>
typename ArrayHandle<T, StorageTagBasic>::template ExecutionTypes<DeviceAdapterTag>::Portal
ArrayHandle<T, StorageTagBasic>::PrepareForOutput(vtkm::Id numVals, DeviceAdapterTag device)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);
  LockType lock = this->GetLock();
  this->PrepareForDevice(lock, device);

  this->Internals->PrepareForOutput(lock, numVals, sizeof(T));
  return PortalFactory<DeviceAdapterTag>::CreatePortal(
    static_cast<T*>(this->Internals->Internals->GetExecutionArray(lock)),
    static_cast<T*>(this->Internals->Internals->GetExecutionArrayEnd(lock)));
}

template <typename T>
template <typename DeviceAdapterTag>
typename ArrayHandle<T, StorageTagBasic>::template ExecutionTypes<DeviceAdapterTag>::Portal
ArrayHandle<T, StorageTagBasic>::PrepareForInPlace(DeviceAdapterTag device)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);
  LockType lock = this->GetLock();
  this->PrepareForDevice(lock, device);

  this->Internals->PrepareForInPlace(lock, sizeof(T));
  return PortalFactory<DeviceAdapterTag>::CreatePortal(
    static_cast<T*>(this->Internals->Internals->GetExecutionArray(lock)),
    static_cast<T*>(this->Internals->Internals->GetExecutionArrayEnd(lock)));
}

template <typename T>
template <typename DeviceAdapterTag>
void ArrayHandle<T, StorageTagBasic>::PrepareForDevice(const LockType& lock,
                                                       DeviceAdapterTag device) const
{
  bool needToRealloc = this->Internals->PrepareForDevice(lock, device, sizeof(T));
  if (needToRealloc)
  {
    this->Internals->Internals->SetExecutionInterface(
      lock,
      new internal::ExecutionArrayInterfaceBasic<DeviceAdapterTag>(
        *(this->Internals->Internals->GetControlArray(lock))));
  }
}

template <typename T>
DeviceAdapterId ArrayHandle<T, StorageTagBasic>::GetDeviceAdapterId() const
{
  LockType lock = this->GetLock();
  return this->Internals->GetDeviceAdapterId(lock);
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::SyncControlArray() const
{
  LockType lock = this->GetLock();
  this->Internals->SyncControlArray(lock, sizeof(T));
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::SyncControlArray(const LockType& lock) const
{
  this->Internals->SyncControlArray(lock, sizeof(T));
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::ReleaseResourcesExecutionInternal(const LockType& lock)
{
  this->Internals->ReleaseResourcesExecutionInternal(lock);
}
}
} // end namespace vtkm::cont


#endif // not vtk_m_cont_internal_ArrayHandleBasicImpl_hxx
