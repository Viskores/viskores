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
  this->SyncControlArray();
  this->Internals->CheckControlArrayValid();
  //CheckControlArrayValid will throw an exception if this->Internals->ControlArrayValid
  //is not valid

  return *(static_cast<StorageType*>(this->Internals->ControlArray));
}

template <typename T>
const typename ArrayHandle<T, StorageTagBasic>::StorageType&
ArrayHandle<T, StorageTagBasic>::GetStorage() const
{
  this->SyncControlArray();
  this->Internals->CheckControlArrayValid();
  //CheckControlArrayValid will throw an exception if this->Internals->ControlArrayValid
  //is not valid

  return *(static_cast<const StorageType*>(this->Internals->ControlArray));
}

template <typename T>
typename ArrayHandle<T, StorageTagBasic>::PortalControl
ArrayHandle<T, StorageTagBasic>::GetPortalControl()
{
  this->SyncControlArray();
  this->Internals->CheckControlArrayValid();
  //CheckControlArrayValid will throw an exception if this->Internals->ControlArrayValid
  //is not valid


  // If the user writes into the iterator we return, then the execution
  // array will become invalid. Play it safe and release the execution
  // resources. (Use the const version to preserve the execution array.)
  this->ReleaseResourcesExecutionInternal();
  StorageType* privStorage = static_cast<StorageType*>(this->Internals->ControlArray);
  return privStorage->GetPortal();
}


template <typename T>
typename ArrayHandle<T, StorageTagBasic>::PortalConstControl
ArrayHandle<T, StorageTagBasic>::GetPortalConstControl() const
{
  this->SyncControlArray();
  this->Internals->CheckControlArrayValid();
  //CheckControlArrayValid will throw an exception if this->Internals->ControlArrayValid
  //is not valid

  StorageType* privStorage = static_cast<StorageType*>(this->Internals->ControlArray);
  return privStorage->GetPortalConst();
}

template <typename T>
vtkm::Id ArrayHandle<T, StorageTagBasic>::GetNumberOfValues() const
{
  return this->Internals->GetNumberOfValues(sizeof(T));
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::Allocate(vtkm::Id numberOfValues)
{
  this->Internals->Allocate(numberOfValues, sizeof(T));
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::Shrink(vtkm::Id numberOfValues)
{
  this->Internals->Shrink(numberOfValues, sizeof(T));
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::ReleaseResourcesExecution()
{
  // Save any data in the execution environment by making sure it is synced
  // with the control environment.
  this->SyncControlArray();
  this->Internals->ReleaseResourcesExecutionInternal();
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::ReleaseResources()
{
  this->Internals->ReleaseResources();
}

template <typename T>
template <typename DeviceAdapterTag>
typename ArrayHandle<T, StorageTagBasic>::template ExecutionTypes<DeviceAdapterTag>::PortalConst
ArrayHandle<T, StorageTagBasic>::PrepareForInput(DeviceAdapterTag device) const
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);
  this->PrepareForDevice(device);

  this->Internals->PrepareForInput(sizeof(T));
  return PortalFactory<DeviceAdapterTag>::CreatePortalConst(
    static_cast<T*>(this->Internals->ExecutionArray),
    static_cast<T*>(this->Internals->ExecutionArrayEnd));
}

template <typename T>
template <typename DeviceAdapterTag>
typename ArrayHandle<T, StorageTagBasic>::template ExecutionTypes<DeviceAdapterTag>::Portal
ArrayHandle<T, StorageTagBasic>::PrepareForOutput(vtkm::Id numVals, DeviceAdapterTag device)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);
  this->PrepareForDevice(device);

  this->Internals->PrepareForOutput(numVals, sizeof(T));
  return PortalFactory<DeviceAdapterTag>::CreatePortal(
    static_cast<T*>(this->Internals->ExecutionArray),
    static_cast<T*>(this->Internals->ExecutionArrayEnd));
}

template <typename T>
template <typename DeviceAdapterTag>
typename ArrayHandle<T, StorageTagBasic>::template ExecutionTypes<DeviceAdapterTag>::Portal
ArrayHandle<T, StorageTagBasic>::PrepareForInPlace(DeviceAdapterTag device)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);
  this->PrepareForDevice(device);

  this->Internals->PrepareForInPlace(sizeof(T));
  return PortalFactory<DeviceAdapterTag>::CreatePortal(
    static_cast<T*>(this->Internals->ExecutionArray),
    static_cast<T*>(this->Internals->ExecutionArrayEnd));
}

template <typename T>
template <typename DeviceAdapterTag>
void ArrayHandle<T, StorageTagBasic>::PrepareForDevice(DeviceAdapterTag device) const
{
  bool needToRealloc = this->Internals->PrepareForDevice(device, sizeof(T));
  if (needToRealloc)
  {
    this->Internals->ExecutionInterface =
      new internal::ExecutionArrayInterfaceBasic<DeviceAdapterTag>(
        *(this->Internals->ControlArray));
  }
}

template <typename T>
DeviceAdapterId ArrayHandle<T, StorageTagBasic>::GetDeviceAdapterId() const
{
  return this->Internals->GetDeviceAdapterId();
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::SyncControlArray() const
{
  this->Internals->SyncControlArray(sizeof(T));
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::ReleaseResourcesExecutionInternal()
{
  this->Internals->ReleaseResourcesExecutionInternal();
}
}
} // end namespace vtkm::cont


#endif // not vtk_m_cont_internal_ArrayHandleBasicImpl_hxx
