//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandle_hxx
#define vtk_m_cont_ArrayHandle_hxx

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace cont
{

template <typename T, typename S>
ArrayHandle<T, S>::InternalStruct::InternalStruct(
  const typename ArrayHandle<T, S>::StorageType& storage)
  : ControlArray(storage)
  , ControlArrayValid(true)
  , ExecutionArrayValid(false)
{
}

template <typename T, typename S>
ArrayHandle<T, S>::InternalStruct::InternalStruct(typename ArrayHandle<T, S>::StorageType&& storage)
  : ControlArray(std::move(storage))
  , ControlArrayValid(true)
  , ExecutionArrayValid(false)
{
}

template <typename T, typename S>
ArrayHandle<T, S>::ArrayHandle()
  : Internals(std::make_shared<InternalStruct>())
{
}

template <typename T, typename S>
ArrayHandle<T, S>::ArrayHandle(const ArrayHandle<T, S>& src)
  : Internals(src.Internals)
{
}

template <typename T, typename S>
ArrayHandle<T, S>::ArrayHandle(ArrayHandle<T, S>&& src) noexcept
  : Internals(std::move(src.Internals))
{
}

template <typename T, typename S>
ArrayHandle<T, S>::ArrayHandle(const typename ArrayHandle<T, S>::StorageType& storage)
  : Internals(std::make_shared<InternalStruct>(storage))
{
}

template <typename T, typename S>
ArrayHandle<T, S>::ArrayHandle(typename ArrayHandle<T, S>::StorageType&& storage) noexcept
  : Internals(std::make_shared<InternalStruct>(std::move(storage)))
{
}

template <typename T, typename S>
ArrayHandle<T, S>::~ArrayHandle()
{
}

template <typename T, typename S>
ArrayHandle<T, S>& ArrayHandle<T, S>::operator=(const ArrayHandle<T, S>& src)
{
  this->Internals = src.Internals;
  return *this;
}

template <typename T, typename S>
ArrayHandle<T, S>& ArrayHandle<T, S>::operator=(ArrayHandle<T, S>&& src) noexcept
{
  this->Internals = std::move(src.Internals);
  return *this;
}

template <typename T, typename S>
typename ArrayHandle<T, S>::StorageType& ArrayHandle<T, S>::GetStorage()
{
  LockType lock = this->GetLock();

  this->SyncControlArray(lock);
  if (this->Internals->IsControlArrayValid(lock))
  {
    return *this->Internals->GetControlArray(lock);
  }
  else
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

template <typename T, typename S>
const typename ArrayHandle<T, S>::StorageType& ArrayHandle<T, S>::GetStorage() const
{
  LockType lock = this->GetLock();

  this->SyncControlArray(lock);
  if (this->Internals->IsControlArrayValid(lock))
  {
    return *this->Internals->GetControlArray(lock);
  }
  else
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

template <typename T, typename S>
typename ArrayHandle<T, S>::PortalControl ArrayHandle<T, S>::GetPortalControl()
{
  LockType lock = this->GetLock();

  this->SyncControlArray(lock);
  if (this->Internals->IsControlArrayValid(lock))
  {
    // If the user writes into the iterator we return, then the execution
    // array will become invalid. Play it safe and release the execution
    // resources. (Use the const version to preserve the execution array.)
    this->ReleaseResourcesExecutionInternal(lock);
    return this->Internals->GetControlArray(lock)->GetPortal();
  }
  else
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

template <typename T, typename S>
typename ArrayHandle<T, S>::PortalConstControl ArrayHandle<T, S>::GetPortalConstControl() const
{
  LockType lock = this->GetLock();

  this->SyncControlArray(lock);
  if (this->Internals->IsControlArrayValid(lock))
  {
    return this->Internals->GetControlArray(lock)->GetPortalConst();
  }
  else
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

template <typename T, typename S>
vtkm::Id ArrayHandle<T, S>::GetNumberOfValues(LockType& lock) const
{
  if (this->Internals->IsControlArrayValid(lock))
  {
    return this->Internals->GetControlArray(lock)->GetNumberOfValues();
  }
  else if (this->Internals->IsExecutionArrayValid(lock))
  {
    return this->Internals->GetExecutionArray(lock)->GetNumberOfValues();
  }
  else
  {
    return 0;
  }
}

template <typename T, typename S>
void ArrayHandle<T, S>::Shrink(vtkm::Id numberOfValues)
{
  VTKM_ASSERT(numberOfValues >= 0);

  if (numberOfValues > 0)
  {
    LockType lock = this->GetLock();

    vtkm::Id originalNumberOfValues = this->GetNumberOfValues(lock);

    if (numberOfValues < originalNumberOfValues)
    {
      if (this->Internals->IsControlArrayValid(lock))
      {
        this->Internals->GetControlArray(lock)->Shrink(numberOfValues);
      }
      if (this->Internals->IsExecutionArrayValid(lock))
      {
        this->Internals->GetExecutionArray(lock)->Shrink(numberOfValues);
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

    VTKM_ASSERT(this->GetNumberOfValues(lock) == numberOfValues);
  }
  else // numberOfValues == 0
  {
    // If we are shrinking to 0, there is nothing to save and we might as well
    // free up memory. Plus, some storage classes expect that data will be
    // deallocated when the size goes to zero.
    this->Allocate(0);
  }
}

template <typename T, typename S>
template <typename DeviceAdapterTag>
typename ArrayHandle<T, S>::template ExecutionTypes<DeviceAdapterTag>::PortalConst
ArrayHandle<T, S>::PrepareForInput(DeviceAdapterTag device) const
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  LockType lock = this->GetLock();

  if (!this->Internals->IsControlArrayValid(lock) && !this->Internals->IsExecutionArrayValid(lock))
  {
    // Want to use an empty array.
    // Set up ArrayHandle state so this actually works.
    this->Internals->GetControlArray(lock)->Allocate(0);
    this->Internals->SetControlArrayValid(lock, true);
  }

  this->PrepareForDevice(lock, device);
  auto portal = this->Internals->GetExecutionArray(lock)->PrepareForInput(
    !this->Internals->IsExecutionArrayValid(lock), device);

  this->Internals->SetExecutionArrayValid(lock, true);

  return portal;
}

template <typename T, typename S>
template <typename DeviceAdapterTag>
typename ArrayHandle<T, S>::template ExecutionTypes<DeviceAdapterTag>::Portal
ArrayHandle<T, S>::PrepareForOutput(vtkm::Id numberOfValues, DeviceAdapterTag device)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  LockType lock = this->GetLock();

  // Invalidate any control arrays.
  // Should the control array resource be released? Probably not a good
  // idea when shared with execution.
  this->Internals->SetControlArrayValid(lock, false);

  this->PrepareForDevice(lock, device);
  auto portal = this->Internals->GetExecutionArray(lock)->PrepareForOutput(numberOfValues, device);

  // We are assuming that the calling code will fill the array using the
  // iterators we are returning, so go ahead and mark the execution array as
  // having valid data. (A previous version of this class had a separate call
  // to mark the array as filled, but that was onerous to call at the the
  // right time and rather pointless since it is basically always the case
  // that the array is going to be filled before anything else. In this
  // implementation the only access to the array is through the iterators
  // returned from this method, so you would have to work to invalidate this
  // assumption anyway.)
  this->Internals->SetExecutionArrayValid(lock, true);

  return portal;
}

template <typename T, typename S>
template <typename DeviceAdapterTag>
typename ArrayHandle<T, S>::template ExecutionTypes<DeviceAdapterTag>::Portal
ArrayHandle<T, S>::PrepareForInPlace(DeviceAdapterTag device)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  LockType lock = this->GetLock();

  if (!this->Internals->IsControlArrayValid(lock) && !this->Internals->IsExecutionArrayValid(lock))
  {
    // Want to use an empty array.
    // Set up ArrayHandle state so this actually works.
    this->Internals->GetControlArray(lock)->Allocate(0);
    this->Internals->SetControlArrayValid(lock, true);
  }

  this->PrepareForDevice(lock, device);
  auto portal = this->Internals->GetExecutionArray(lock)->PrepareForInPlace(
    !this->Internals->IsExecutionArrayValid(lock), device);

  this->Internals->SetExecutionArrayValid(lock, true);

  // Invalidate any control arrays since their data will become invalid when
  // the execution data is overwritten. Don't actually release the control
  // array. It may be shared as the execution array.
  this->Internals->SetControlArrayValid(lock, false);

  return portal;
}

template <typename T, typename S>
template <typename DeviceAdapterTag>
void ArrayHandle<T, S>::PrepareForDevice(LockType& lock, DeviceAdapterTag device) const
{
  if (this->Internals->GetExecutionArray(lock) != nullptr)
  {
    if (this->Internals->GetExecutionArray(lock)->IsDeviceAdapter(DeviceAdapterTag()))
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
      this->SyncControlArray(lock);
      // Need to change some state that does not change the logical state from
      // an external point of view.
      this->Internals->DeleteExecutionArray(lock);
    }
  }

  // Need to change some state that does not change the logical state from
  // an external point of view.
  this->Internals->NewExecutionArray(lock, device);
}

template <typename T, typename S>
void ArrayHandle<T, S>::SyncControlArray(LockType& lock) const
{
  if (!this->Internals->IsControlArrayValid(lock))
  {
    // Need to change some state that does not change the logical state from
    // an external point of view.
    if (this->Internals->IsExecutionArrayValid(lock))
    {
      this->Internals->GetExecutionArray(lock)->RetrieveOutputData(
        this->Internals->GetControlArray(lock));
      this->Internals->SetControlArrayValid(lock, true);
    }
    else
    {
      // This array is in the null state (there is nothing allocated), but
      // the calling function wants to do something with the array. Put this
      // class into a valid state by allocating an array of size 0.
      this->Internals->GetControlArray(lock)->Allocate(0);
      this->Internals->SetControlArrayValid(lock, true);
    }
  }
}
}
} // vtkm::cont

namespace vtkm
{
namespace cont
{
namespace internal
{

namespace detail
{
template <typename ArrayHandle>
inline void VTKM_CONT StorageSerialization(vtkmdiy::BinaryBuffer& bb,
                                           const ArrayHandle& obj,
                                           std::false_type)
{
  vtkm::Id count = obj.GetNumberOfValues();
  vtkmdiy::save(bb, count);

  vtkmdiy::save(bb, vtkm::Id(0)); //not a basic storage
  auto portal = obj.GetPortalConstControl();
  for (vtkm::Id i = 0; i < count; ++i)
  {
    vtkmdiy::save(bb, portal.Get(i));
  }
}

template <typename ArrayHandle>
inline void VTKM_CONT StorageSerialization(vtkmdiy::BinaryBuffer& bb,
                                           const ArrayHandle& obj,
                                           std::true_type)
{
  vtkm::Id count = obj.GetNumberOfValues();
  vtkmdiy::save(bb, count);

  vtkmdiy::save(bb, vtkm::Id(1)); //is basic storage
  vtkmdiy::save(bb, obj.GetStorage().GetArray(), static_cast<std::size_t>(count));
}
}

template <typename T, typename S>
inline void VTKM_CONT ArrayHandleDefaultSerialization(vtkmdiy::BinaryBuffer& bb,
                                                      const vtkm::cont::ArrayHandle<T, S>& obj)
{
  using is_basic = typename std::is_same<S, vtkm::cont::StorageTagBasic>::type;
  detail::StorageSerialization(bb, obj, is_basic{});
}
}
}
} // vtkm::cont::internal

namespace mangled_diy_namespace
{

template <typename T>
VTKM_CONT void Serialization<vtkm::cont::ArrayHandle<T>>::load(BinaryBuffer& bb,
                                                               vtkm::cont::ArrayHandle<T>& obj)
{
  vtkm::Id count = 0;
  vtkmdiy::load(bb, count);
  obj.Allocate(count);

  vtkm::Id input_was_basic_storage = 0;
  vtkmdiy::load(bb, input_was_basic_storage);
  if (input_was_basic_storage)
  {
    vtkmdiy::load(bb, obj.GetStorage().GetArray(), static_cast<std::size_t>(count));
  }
  else
  {
    auto portal = obj.GetPortalControl();
    for (vtkm::Id i = 0; i < count; ++i)
    {
      T val{};
      vtkmdiy::load(bb, val);
      portal.Set(i, val);
    }
  }
}
} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandle_hxx
