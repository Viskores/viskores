//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

namespace vtkm {
namespace cont {

template<typename T, typename S>
ArrayHandle<T,S>::ArrayHandle()
  : Internals(new InternalStruct)
{
  this->Internals->ControlArrayValid = false;
  this->Internals->ExecutionArrayValid = false;
}

template<typename T, typename S>
ArrayHandle<T,S>::ArrayHandle(const ArrayHandle<T,S> &src)
  : Internals(src.Internals)
{  }

template<typename T, typename S>
ArrayHandle<T,S>::ArrayHandle(ArrayHandle<T,S> &&src)
  : Internals(std::move(src.Internals))
{  }

template<typename T, typename S>
ArrayHandle<T,S>::ArrayHandle(const typename ArrayHandle<T,S>::StorageType &storage)
  : Internals(new InternalStruct)
{
  this->Internals->ControlArray = storage;
  this->Internals->ControlArrayValid = true;
  this->Internals->ExecutionArrayValid = false;
}

template<typename T, typename S>
ArrayHandle<T,S>::~ArrayHandle()
{
}

template<typename T, typename S>
ArrayHandle<T,S>&
ArrayHandle<T,S>::operator=(const ArrayHandle<T,S> &src)
{
  this->Internals = src.Internals;
  return *this;
}

template<typename T, typename S>
ArrayHandle<T,S>&
ArrayHandle<T,S>::operator=(ArrayHandle<T,S> &&src)
{
  this->Internals = std::move(src.Internals);
  return *this;
}

template<typename T, typename S>
typename ArrayHandle<T,S>::StorageType&
ArrayHandle<T,S>::GetStorage()
{
  this->SyncControlArray();
  if (this->Internals->ControlArrayValid)
  {
    return this->Internals->ControlArray;
  }
  else
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

template<typename T, typename S>
const typename ArrayHandle<T,S>::StorageType&
ArrayHandle<T,S>::GetStorage() const
{
  this->SyncControlArray();
  if (this->Internals->ControlArrayValid)
  {
    return this->Internals->ControlArray;
  }
  else
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

template<typename T, typename S>
typename ArrayHandle<T,S>::PortalControl
ArrayHandle<T,S>::GetPortalControl()
{
  this->SyncControlArray();
  if (this->Internals->ControlArrayValid)
  {
    // If the user writes into the iterator we return, then the execution
    // array will become invalid. Play it safe and release the execution
    // resources. (Use the const version to preserve the execution array.)
    this->ReleaseResourcesExecutionInternal();
    return this->Internals->ControlArray.GetPortal();
  }
  else
  {
    throw vtkm::cont::ErrorInternal(
          "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

template<typename T, typename S>
typename ArrayHandle<T,S>::PortalConstControl
ArrayHandle<T,S>::GetPortalConstControl() const
{
  this->SyncControlArray();
  if (this->Internals->ControlArrayValid)
  {
    return this->Internals->ControlArray.GetPortalConst();
  }
  else
  {
    throw vtkm::cont::ErrorInternal(
          "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

template<typename T, typename S>
vtkm::Id ArrayHandle<T,S>::GetNumberOfValues() const
{
  if (this->Internals->ControlArrayValid)
  {
    return this->Internals->ControlArray.GetNumberOfValues();
  }
  else if (this->Internals->ExecutionArrayValid)
  {
    return this->Internals->ExecutionArray->GetNumberOfValues();
  }
  else
  {
    return 0;
  }
}

template<typename T, typename S>
template<typename IteratorType, typename DeviceAdapterTag>
void ArrayHandle<T,S>::CopyInto(IteratorType dest, DeviceAdapterTag) const
{
  using pointer_type = typename std::iterator_traits<IteratorType>::pointer;
  using value_type = typename std::remove_pointer<pointer_type>::type;

  static_assert( !std::is_const<value_type>::value,
                 "CopyInto requires a non const iterator." );

  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  if (!this->Internals->ControlArrayValid &&
      !this->Internals->ExecutionArrayValid)
    {
    throw vtkm::cont::ErrorBadValue(
      "ArrayHandle has no data to copy into Iterator.");
    }

  if (!this->Internals->ControlArrayValid &&
      this->Internals->ExecutionArray->IsDeviceAdapter(DeviceAdapterTag()))
    {
      /// Dynamically cast ArrayHandleExecutionManagerBase into a concrete
      /// class and call CopyInto. The dynamic conversion will be sucessful
      /// becuase the check to ensure the ExecutionArray is of the type
      /// DeviceAdapterTag has already passed
      typedef vtkm::cont::internal::ArrayHandleExecutionManager<
                              T, StorageTag, DeviceAdapterTag> ConcreteType;
      ConcreteType *ConcreteExecutionArray =
                dynamic_cast<ConcreteType*>(this->Internals->ExecutionArray.get());

      ConcreteExecutionArray->CopyInto(dest);
    }
  else
    {
    PortalConstControl portal = this->GetPortalConstControl();
    std::copy(vtkm::cont::ArrayPortalToIteratorBegin(portal),
              vtkm::cont::ArrayPortalToIteratorEnd(portal),
              dest);
    }
}

template<typename T, typename S>
void ArrayHandle<T,S>::Shrink(vtkm::Id numberOfValues)
{
  VTKM_ASSERT(numberOfValues >= 0);

  if (numberOfValues > 0)
  {
    vtkm::Id originalNumberOfValues = this->GetNumberOfValues();

    if (numberOfValues < originalNumberOfValues)
    {
      if (this->Internals->ControlArrayValid)
      {
        this->Internals->ControlArray.Shrink(numberOfValues);
      }
      if (this->Internals->ExecutionArrayValid)
      {
        this->Internals->ExecutionArray->Shrink(numberOfValues);
      }
    }
    else if (numberOfValues == originalNumberOfValues)
    {
      // Nothing to do.
    }
    else // numberOfValues > originalNumberOfValues
    {
      throw vtkm::cont::ErrorBadValue(
            "ArrayHandle::Shrink cannot be used to grow array.");
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

template<typename T, typename S>
template<typename DeviceAdapterTag>
typename ArrayHandle<T,S>::template ExecutionTypes<DeviceAdapterTag>::PortalConst
ArrayHandle<T,S>::PrepareForInput(DeviceAdapterTag) const
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  if (!this->Internals->ControlArrayValid
      && !this->Internals->ExecutionArrayValid)
  {
    // Want to use an empty array.
    // Set up ArrayHandle state so this actually works.
    this->Internals->ControlArray.Allocate(0);
    this->Internals->ControlArrayValid = true;
  }

  this->PrepareForDevice(DeviceAdapterTag());
  typename ExecutionTypes<DeviceAdapterTag>::PortalConst portal =
      this->Internals->ExecutionArray->PrepareForInput(
        !this->Internals->ExecutionArrayValid, DeviceAdapterTag());

  this->Internals->ExecutionArrayValid = true;

  return portal;
}

template<typename T, typename S>
template<typename DeviceAdapterTag>
typename ArrayHandle<T,S>::template ExecutionTypes<DeviceAdapterTag>::Portal
ArrayHandle<T,S>::PrepareForOutput(vtkm::Id numberOfValues, DeviceAdapterTag)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  // Invalidate any control arrays.
  // Should the control array resource be released? Probably not a good
  // idea when shared with execution.
  this->Internals->ControlArrayValid = false;

  this->PrepareForDevice(DeviceAdapterTag());
  typename ExecutionTypes<DeviceAdapterTag>::Portal portal =
      this->Internals->ExecutionArray->PrepareForOutput(numberOfValues,
                                                        DeviceAdapterTag());

  // We are assuming that the calling code will fill the array using the
  // iterators we are returning, so go ahead and mark the execution array as
  // having valid data. (A previous version of this class had a separate call
  // to mark the array as filled, but that was onerous to call at the the
  // right time and rather pointless since it is basically always the case
  // that the array is going to be filled before anything else. In this
  // implementation the only access to the array is through the iterators
  // returned from this method, so you would have to work to invalidate this
  // assumption anyway.)
  this->Internals->ExecutionArrayValid = true;

  return portal;
}

template<typename T, typename S>
template<typename DeviceAdapterTag>
typename ArrayHandle<T,S>::template ExecutionTypes<DeviceAdapterTag>::Portal
ArrayHandle<T,S>::PrepareForInPlace(DeviceAdapterTag)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  if (!this->Internals->ControlArrayValid
      && !this->Internals->ExecutionArrayValid)
  {
    // Want to use an empty array.
    // Set up ArrayHandle state so this actually works.
    this->Internals->ControlArray.Allocate(0);
    this->Internals->ControlArrayValid = true;
  }

  this->PrepareForDevice(DeviceAdapterTag());
  typename ExecutionTypes<DeviceAdapterTag>::Portal portal =
      this->Internals->ExecutionArray->PrepareForInPlace(
        !this->Internals->ExecutionArrayValid, DeviceAdapterTag());

  this->Internals->ExecutionArrayValid = true;

  // Invalidate any control arrays since their data will become invalid when
  // the execution data is overwritten. Don't actually release the control
  // array. It may be shared as the execution array.
  this->Internals->ControlArrayValid = false;

  return portal;
}

template<typename T, typename S>
template<typename DeviceAdapterTag>
void ArrayHandle<T,S>::PrepareForDevice(DeviceAdapterTag) const
{
  if (this->Internals->ExecutionArray != nullptr)
  {
    if (this->Internals->ExecutionArray->IsDeviceAdapter(DeviceAdapterTag()))
    {
      // Already have manager for correct device adapter. Nothing to do.
      return;
    }
    else
    {
      // Have the wrong manager. Delete the old one and create a new one
      // of the right type. (BTW, it would be possible for the array handle
      // to hold references to execution arrays on multiple devices. However,
      // there is not a clear use case for that yet and it is unclear what
      // the behavior of "dirty" arrays should be, so it is not currently
      // implemented.)
      this->SyncControlArray();
      // Need to change some state that does not change the logical state from
      // an external point of view.
      InternalStruct *internals
          = const_cast<InternalStruct*>(this->Internals.get());
      internals->ExecutionArray.reset();
      internals->ExecutionArrayValid = false;
      }
    }

  VTKM_ASSERT(this->Internals->ExecutionArray == nullptr);
  VTKM_ASSERT(!this->Internals->ExecutionArrayValid);
  // Need to change some state that does not change the logical state from
  // an external point of view.
  InternalStruct *internals
      = const_cast<InternalStruct*>(this->Internals.get());
  internals->ExecutionArray.reset(
        new vtkm::cont::internal::ArrayHandleExecutionManager<
          T, StorageTag, DeviceAdapterTag>(&internals->ControlArray));
}


template<typename T, typename S>
void ArrayHandle<T,S>::SyncControlArray() const
{
  if (!this->Internals->ControlArrayValid)
  {
    // Need to change some state that does not change the logical state from
    // an external point of view.
    InternalStruct *internals
      = const_cast<InternalStruct*>(this->Internals.get());
    if (this->Internals->ExecutionArrayValid)
    {
      internals->ExecutionArray->RetrieveOutputData(&internals->ControlArray);
      internals->ControlArrayValid = true;
    }
    else
    {
      // This array is in the null state (there is nothing allocated), but
      // the calling function wants to do something with the array. Put this
      // class into a valid state by allocating an array of size 0.
      internals->ControlArray.Allocate(0);
      internals->ControlArrayValid = true;
    }
  }
}


}
}
