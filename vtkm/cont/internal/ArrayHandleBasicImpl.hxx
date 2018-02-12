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
ArrayHandle<T, StorageTagBasic>::ArrayHandle(const Thisclass&& src)
  : Internals(std::move(src.Internals))
{
}

template <typename T>
ArrayHandle<T, StorageTagBasic>::ArrayHandle(const StorageType& storage)
  : Internals(new internal::ArrayHandleImpl(storage))
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
ArrayHandle<T, StorageTagBasic>& ArrayHandle<T, StorageTagBasic>::operator=(Thisclass&& src)
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
  if (this->Internals->ControlArrayValid)
  {
    return *(static_cast<StorageType*>(this->Internals->ControlArray));
  }
  else
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

template <typename T>
const typename ArrayHandle<T, StorageTagBasic>::StorageType&
ArrayHandle<T, StorageTagBasic>::GetStorage() const
{
  this->SyncControlArray();
  if (this->Internals->ControlArrayValid)
  {
    return *(static_cast<const StorageType*>(this->Internals->ControlArray));
  }
  else
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

template <typename T>
typename ArrayHandle<T, StorageTagBasic>::PortalControl
ArrayHandle<T, StorageTagBasic>::GetPortalControl()
{
  this->SyncControlArray();
  if (this->Internals->ControlArrayValid)
  {
    // If the user writes into the iterator we return, then the execution
    // array will become invalid. Play it safe and release the execution
    // resources. (Use the const version to preserve the execution array.)
    this->ReleaseResourcesExecutionInternal();
    StorageType* privStorage = static_cast<StorageType*>(this->Internals->ControlArray);
    return privStorage->GetPortal();
  }
  else
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}


template <typename T>
typename ArrayHandle<T, StorageTagBasic>::PortalConstControl
ArrayHandle<T, StorageTagBasic>::GetPortalConstControl() const
{
  this->SyncControlArray();
  if (this->Internals->ControlArrayValid)
  {
    StorageType* privStorage = static_cast<StorageType*>(this->Internals->ControlArray);
    return privStorage->GetPortalConst();
  }
  else
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

template <typename T>
vtkm::Id ArrayHandle<T, StorageTagBasic>::GetNumberOfValues() const
{
  if (this->Internals->ControlArrayValid)
  {
    return this->Internals->ControlArray->GetNumberOfValues();
  }
  else if (this->Internals->ExecutionArrayValid)
  {
    return static_cast<vtkm::Id>(static_cast<T*>(this->Internals->ExecutionArrayEnd) -
                                 static_cast<T*>(this->Internals->ExecutionArray));
  }
  else
  {
    return 0;
  }
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::Allocate(vtkm::Id numberOfValues)
{
  this->ReleaseResourcesExecutionInternal();
  this->Internals->ControlArray->AllocateValues(numberOfValues, sizeof(T));
  this->Internals->ControlArrayValid = true;
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::Shrink(vtkm::Id numberOfValues)
{
  VTKM_ASSERT(numberOfValues >= 0);

  if (numberOfValues > 0)
  {
    vtkm::Id originalNumberOfValues = this->GetNumberOfValues();

    if (numberOfValues < originalNumberOfValues)
    {
      if (this->Internals->ControlArrayValid)
      {
        this->Internals->ControlArray->Shrink(numberOfValues);
      }
      if (this->Internals->ExecutionArrayValid)
      {
        this->Internals->ExecutionArrayEnd =
          static_cast<T*>(this->Internals->ExecutionArray) + numberOfValues;
      }
    }
    else if (numberOfValues == originalNumberOfValues)
    {
      // Nothing to do.
    }
    else // numberOfValues > originalNumberOfValues
    {
      throw vtkm::cont::ErrorBadValue("ArrayHandle::Shrink cannot be used to grow array.");
    }

    VTKM_ASSERT(this->GetNumberOfValues() == numberOfValues);
  }
  else // numberOfValues == 0
  {
    // If we are shrinking to 0, there is nothing to save and we might as well
    // free up memory. Plus, some storage classes expect that data will be
    // deallocated when the size goes to zero.
    this->Allocate(0);
  }
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::ReleaseResourcesExecution()
{
  // Save any data in the execution environment by making sure it is synced
  // with the control environment.
  this->SyncControlArray();
  this->ReleaseResourcesExecutionInternal();
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::ReleaseResources()
{
  this->ReleaseResourcesExecutionInternal();

  if (this->Internals->ControlArrayValid)
  {
    this->Internals->ControlArray->ReleaseResources();
    this->Internals->ControlArrayValid = false;
  }
}

template <typename T>
template <typename DeviceAdapterTag>
typename ArrayHandle<T, StorageTagBasic>::template ExecutionTypes<DeviceAdapterTag>::PortalConst
ArrayHandle<T, StorageTagBasic>::PrepareForInput(DeviceAdapterTag device) const
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);
  internal::ArrayHandleImpl* priv = const_cast<internal::ArrayHandleImpl*>(this->Internals.get());

  this->PrepareForDevice(device);
  const vtkm::Id numVals = this->GetNumberOfValues();
  const vtkm::UInt64 numBytes = sizeof(T) * static_cast<vtkm::UInt64>(numVals);


  if (!this->Internals->ExecutionArrayValid)
  {
    // Initialize an empty array if needed:
    if (!this->Internals->ControlArrayValid)
    {
      priv->ControlArray->AllocateValues(0, sizeof(T));
      this->Internals->ControlArrayValid = true;
    }

    internal::TypelessExecutionArray execArray(priv->ExecutionArray,
                                               priv->ExecutionArrayEnd,
                                               priv->ExecutionArrayCapacity,
                                               priv->ControlArray->GetBasePointer(),
                                               priv->ControlArray->GetCapacityPointer());

    priv->ExecutionInterface->Allocate(execArray, numVals, sizeof(T));

    priv->ExecutionInterface->CopyFromControl(
      priv->ControlArray->GetBasePointer(), priv->ExecutionArray, numBytes);

    this->Internals->ExecutionArrayValid = true;
  }
  this->Internals->ExecutionInterface->UsingForRead(
    priv->ControlArray->GetBasePointer(), priv->ExecutionArray, numBytes);

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
  internal::ArrayHandleImpl* priv = const_cast<internal::ArrayHandleImpl*>(this->Internals.get());

  this->PrepareForDevice(device);

  // Invalidate control arrays since we expect the execution data to be
  // overwritten. Don't free control resources in case they're shared with
  // the execution environment.
  this->Internals->ControlArrayValid = false;

  internal::TypelessExecutionArray execArray(priv->ExecutionArray,
                                             priv->ExecutionArrayEnd,
                                             priv->ExecutionArrayCapacity,
                                             priv->ControlArray->GetBasePointer(),
                                             priv->ControlArray->GetCapacityPointer());

  this->Internals->ExecutionInterface->Allocate(execArray, numVals, sizeof(T));
  const vtkm::UInt64 numBytes = sizeof(T) * static_cast<vtkm::UInt64>(numVals);
  this->Internals->ExecutionInterface->UsingForWrite(
    priv->ControlArray->GetBasePointer(), priv->ExecutionArray, numBytes);

  this->Internals->ExecutionArrayValid = true;


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
  internal::ArrayHandleImpl* priv = const_cast<internal::ArrayHandleImpl*>(this->Internals.get());

  this->PrepareForDevice(device);
  const vtkm::Id numVals = this->GetNumberOfValues();
  const vtkm::UInt64 numBytes = sizeof(T) * static_cast<vtkm::UInt64>(numVals);

  if (!this->Internals->ExecutionArrayValid)
  {
    // Initialize an empty array if needed:
    if (!this->Internals->ControlArrayValid)
    {
      priv->ControlArray->AllocateValues(0, sizeof(T));
      this->Internals->ControlArrayValid = true;
    }

    internal::TypelessExecutionArray execArray(this->Internals->ExecutionArray,
                                               this->Internals->ExecutionArrayEnd,
                                               this->Internals->ExecutionArrayCapacity,
                                               priv->ControlArray->GetBasePointer(),
                                               priv->ControlArray->GetCapacityPointer());

    priv->ExecutionInterface->Allocate(execArray, numVals, sizeof(T));

    priv->ExecutionInterface->CopyFromControl(
      priv->ControlArray->GetBasePointer(), priv->ExecutionArray, numBytes);

    this->Internals->ExecutionArrayValid = true;
  }

  priv->ExecutionInterface->UsingForReadWrite(
    priv->ControlArray->GetBasePointer(), priv->ExecutionArray, numBytes);

  // Invalidate the control array, since we expect the values to be modified:
  this->Internals->ControlArrayValid = false;

  return PortalFactory<DeviceAdapterTag>::CreatePortal(
    static_cast<T*>(this->Internals->ExecutionArray),
    static_cast<T*>(this->Internals->ExecutionArrayEnd));
}

template <typename T>
template <typename DeviceAdapterTag>
void ArrayHandle<T, StorageTagBasic>::PrepareForDevice(DeviceAdapterTag) const
{
  DeviceAdapterId devId = DeviceAdapterTraits<DeviceAdapterTag>::GetId();
  internal::ArrayHandleImpl* priv = const_cast<internal::ArrayHandleImpl*>(this->Internals.get());
  // Check if the current device matches the last one and sync through
  // the control environment if the device changes.
  if (this->Internals->ExecutionInterface)
  {
    if (this->Internals->ExecutionInterface->GetDeviceId() == devId)
    {
      // All set, nothing to do.
      return;
    }
    else
    {
      // Update the device allocator:
      this->SyncControlArray();
      internal::TypelessExecutionArray execArray(priv->ExecutionArray,
                                                 priv->ExecutionArrayEnd,
                                                 priv->ExecutionArrayCapacity,
                                                 priv->ControlArray->GetBasePointer(),
                                                 priv->ControlArray->GetCapacityPointer());
      priv->ExecutionInterface->Free(execArray);
      delete priv->ExecutionInterface;
      priv->ExecutionInterface = nullptr;
      priv->ExecutionArrayValid = false;
    }
  }

  VTKM_ASSERT(priv->ExecutionInterface == nullptr);
  VTKM_ASSERT(!priv->ExecutionArrayValid);

  priv->ExecutionInterface =
    new internal::ExecutionArrayInterfaceBasic<DeviceAdapterTag>(*this->Internals->ControlArray);
}

template <typename T>
DeviceAdapterId ArrayHandle<T, StorageTagBasic>::GetDeviceAdapterId() const
{
  return this->Internals->ExecutionArrayValid ? this->Internals->ExecutionInterface->GetDeviceId()
                                              : VTKM_DEVICE_ADAPTER_UNDEFINED;
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::SyncControlArray() const
{
  if (!this->Internals->ControlArrayValid)
  {
    // Need to change some state that does not change the logical state from
    // an external point of view.
    internal::ArrayHandleImpl* priv = const_cast<internal::ArrayHandleImpl*>(this->Internals.get());
    if (this->Internals->ExecutionArrayValid)
    {
      const vtkm::Id numVals = static_cast<vtkm::Id>(static_cast<T*>(priv->ExecutionArrayEnd) -
                                                     static_cast<T*>(priv->ExecutionArray));
      const vtkm::UInt64 numBytes = sizeof(T) * static_cast<vtkm::UInt64>(numVals);
      priv->ControlArray->AllocateValues(numVals, sizeof(T));
      priv->ExecutionInterface->CopyToControl(
        priv->ExecutionArray, static_cast<T*>(priv->ControlArray->GetBasePointer()), numBytes);
      priv->ControlArrayValid = true;
    }
    else
    {
      // This array is in the null state (there is nothing allocated), but
      // the calling function wants to do something with the array. Put this
      // class into a valid state by allocating an array of size 0.
      priv->ControlArray->AllocateValues(0, sizeof(T));
      priv->ControlArrayValid = true;
    }
  }
}

template <typename T>
void ArrayHandle<T, StorageTagBasic>::ReleaseResourcesExecutionInternal()
{
  if (this->Internals->ExecutionArrayValid)
  {
    internal::TypelessExecutionArray execArray(this->Internals->ExecutionArray,
                                               this->Internals->ExecutionArrayEnd,
                                               this->Internals->ExecutionArrayCapacity,
                                               this->Internals->ControlArray->GetBasePointer(),
                                               this->Internals->ControlArray->GetCapacityPointer());
    this->Internals->ExecutionInterface->Free(execArray);
    this->Internals->ExecutionArrayValid = false;
  }
}
}
} // end namespace vtkm::cont


#endif // not vtk_m_cont_internal_ArrayHandleBasicImpl_hxx
