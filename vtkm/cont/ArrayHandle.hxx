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
  , ControlArrayValid(new bool(true))
  , ExecutionArrayValid(false)
{
}

template <typename T, typename S>
ArrayHandle<T, S>::InternalStruct::InternalStruct(typename ArrayHandle<T, S>::StorageType&& storage)
  : ControlArray(std::move(storage))
  , ControlArrayValid(new bool(true))
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
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->GetLock();

    this->SyncControlArray(lock, token);
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
}

template <typename T, typename S>
const typename ArrayHandle<T, S>::StorageType& ArrayHandle<T, S>::GetStorage() const
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->GetLock();

    this->SyncControlArray(lock, token);
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
}

template <typename T, typename S>
typename ArrayHandle<T, S>::StorageType::PortalType ArrayHandle<T, S>::GetPortalControl()
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->GetLock();

    this->SyncControlArray(lock, token);
    if (this->Internals->IsControlArrayValid(lock))
    {
      // If the user writes into the iterator we return, then the execution
      // array will become invalid. Play it safe and release the execution
      // resources. (Use the const version to preserve the execution array.)
      this->ReleaseResourcesExecutionInternal(lock, token);
      return this->Internals->GetControlArray(lock)->GetPortal();
    }
    else
    {
      throw vtkm::cont::ErrorInternal(
        "ArrayHandle::SyncControlArray did not make control array valid.");
    }
  }
}

template <typename T, typename S>
typename ArrayHandle<T, S>::StorageType::PortalConstType ArrayHandle<T, S>::GetPortalConstControl()
  const
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->GetLock();

    this->SyncControlArray(lock, token);
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
}

template <typename T, typename S>
typename ArrayHandle<T, S>::ReadPortalType ArrayHandle<T, S>::ReadPortal() const
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->GetLock();
    this->WaitToRead(lock, token);

    this->SyncControlArray(lock, token);
    if (this->Internals->IsControlArrayValid(lock))
    {
      return ReadPortalType(this->Internals->GetControlArrayValidPointer(lock),
                            this->Internals->GetControlArray(lock)->GetPortalConst());
    }
    else
    {
      throw vtkm::cont::ErrorInternal(
        "ArrayHandle::SyncControlArray did not make control array valid.");
    }
  }
}

template <typename T, typename S>
typename ArrayHandle<T, S>::WritePortalType ArrayHandle<T, S>::WritePortal() const
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->GetLock();
    this->WaitToWrite(lock, token);

    this->SyncControlArray(lock, token);
    if (this->Internals->IsControlArrayValid(lock))
    {
      // If the user writes into the iterator we return, then the execution
      // array will become invalid. Play it safe and release the execution
      // resources. (Use the const version to preserve the execution array.)
      this->ReleaseResourcesExecutionInternal(lock, token);
      return WritePortalType(this->Internals->GetControlArrayValidPointer(lock),
                             this->Internals->GetControlArray(lock)->GetPortal());
    }
    else
    {
      throw vtkm::cont::ErrorInternal(
        "ArrayHandle::SyncControlArray did not make control array valid.");
    }
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

  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;

  if (numberOfValues > 0)
  {
    LockType lock = this->GetLock();

    vtkm::Id originalNumberOfValues = this->GetNumberOfValues(lock);

    if (numberOfValues < originalNumberOfValues)
    {
      this->WaitToWrite(lock, token);
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
ArrayHandle<T, S>::PrepareForInput(DeviceAdapterTag device, vtkm::cont::Token& token) const
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  LockType lock = this->GetLock();
  this->WaitToRead(lock, token);

  if (!this->Internals->IsControlArrayValid(lock) && !this->Internals->IsExecutionArrayValid(lock))
  {
    // Want to use an empty array.
    // Set up ArrayHandle state so this actually works.
    this->Internals->GetControlArray(lock)->Allocate(0);
    this->Internals->SetControlArrayValid(lock, true);
  }

  this->PrepareForDevice(lock, token, device);
  auto portal = this->Internals->GetExecutionArray(lock)->PrepareForInput(
    !this->Internals->IsExecutionArrayValid(lock), device, token);

  this->Internals->SetExecutionArrayValid(lock, true);

  return portal;
}

template <typename T, typename S>
template <typename DeviceAdapterTag>
typename ArrayHandle<T, S>::template ExecutionTypes<DeviceAdapterTag>::Portal
ArrayHandle<T, S>::PrepareForOutput(vtkm::Id numberOfValues,
                                    DeviceAdapterTag device,
                                    vtkm::cont::Token& token)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  LockType lock = this->GetLock();
  this->WaitToWrite(lock, token);

  // Invalidate any control arrays.
  // Should the control array resource be released? Probably not a good
  // idea when shared with execution.
  this->Internals->SetControlArrayValid(lock, false);

  this->PrepareForDevice(lock, token, device);
  auto portal =
    this->Internals->GetExecutionArray(lock)->PrepareForOutput(numberOfValues, device, token);

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
ArrayHandle<T, S>::PrepareForInPlace(DeviceAdapterTag device, vtkm::cont::Token& token)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  LockType lock = this->GetLock();
  this->WaitToWrite(lock, token);

  if (!this->Internals->IsControlArrayValid(lock) && !this->Internals->IsExecutionArrayValid(lock))
  {
    // Want to use an empty array.
    // Set up ArrayHandle state so this actually works.
    this->Internals->GetControlArray(lock)->Allocate(0);
    this->Internals->SetControlArrayValid(lock, true);
  }

  this->PrepareForDevice(lock, token, device);
  auto portal = this->Internals->GetExecutionArray(lock)->PrepareForInPlace(
    !this->Internals->IsExecutionArrayValid(lock), device, token);

  this->Internals->SetExecutionArrayValid(lock, true);

  // Invalidate any control arrays since their data will become invalid when
  // the execution data is overwritten. Don't actually release the control
  // array. It may be shared as the execution array.
  this->Internals->SetControlArrayValid(lock, false);

  return portal;
}

template <typename T, typename S>
template <typename DeviceAdapterTag>
void ArrayHandle<T, S>::PrepareForDevice(LockType& lock,
                                         vtkm::cont::Token& token,
                                         DeviceAdapterTag device) const
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
      // of the right type. (TODO: it would be possible for the array handle
      // to hold references to execution arrays on multiple devices. When data
      // are written on one devices, all the other devices should get cleared.)

      // BUG: There is a non-zero chance that while waiting for the write lock, another thread
      // could change the ExecutionInterface, which would cause problems. In the future we should
      // support multiple devices, in which case we would not have to delete one execution array
      // to load another.
      // BUG: The current implementation does not allow the ArrayHandle to be on two devices
      // at the same time. Thus, it is not possible for two simultaneously read from the same
      // ArrayHandle on two different devices. This might cause unexpected deadlocks.
      this->WaitToWrite(lock, token, true); // Make sure no one is reading device array
      this->SyncControlArray(lock, token);
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
void ArrayHandle<T, S>::SyncControlArray(LockType& lock, vtkm::cont::Token& token) const
{
  if (!this->Internals->IsControlArrayValid(lock))
  {
    // It may be the case that `SyncControlArray` is called from a method that has a `Token`.
    // However, if we are here, that `Token` should not already be attached to this array.
    // If it were, then there should be no reason to move data arround (unless the `Token`
    // was used when preparing for multiple devices, which it should not be used like that).
    this->WaitToRead(lock, token);

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

template <typename T, typename S>
bool ArrayHandle<T, S>::CanRead(const LockType& lock, const vtkm::cont::Token& token) const
{
  // If the token is already attached to this array, then we allow reading.
  if (token.IsAttached(this->Internals->GetWriteCount(lock)) ||
      token.IsAttached(this->Internals->GetReadCount(lock)))
  {
    return true;
  }

  // If there is anyone else waiting at the top of the queue, we cannot access this array.
  auto& queue = this->Internals->GetQueue(lock);
  if (!queue.empty() && (queue.front() != token))
  {
    return false;
  }

  // No one else is waiting, so we can read the array as long as no one else is writing.
  return (*this->Internals->GetWriteCount(lock) < 1);
}

template <typename T, typename S>
bool ArrayHandle<T, S>::CanWrite(const LockType& lock, const vtkm::cont::Token& token) const
{
  // If the token is already attached to this array, then we allow writing.
  if (token.IsAttached(this->Internals->GetWriteCount(lock)) ||
      token.IsAttached(this->Internals->GetReadCount(lock)))
  {
    return true;
  }

  // If there is anyone else waiting at the top of the queue, we cannot access this array.
  auto& queue = this->Internals->GetQueue(lock);
  if (!queue.empty() && (queue.front() != token))
  {
    return false;
  }

  // No one else is waiting, so we can write the array as long as no one else is reading or writing.
  return ((*this->Internals->GetWriteCount(lock) < 1) &&
          (*this->Internals->GetReadCount(lock) < 1));
}

template <typename T, typename S>
void ArrayHandle<T, S>::WaitToRead(LockType& lock, vtkm::cont::Token& token) const
{
  this->Enqueue(lock, token);

  // Note that if you deadlocked here, that means that you are trying to do a read operation on an
  // array where an object is writing to it.
  this->Internals->ConditionVariable.wait(
    lock, [&lock, &token, this] { return this->CanRead(lock, token); });

  token.Attach(this->Internals,
               this->Internals->GetReadCount(lock),
               lock,
               &this->Internals->ConditionVariable);

  // We successfully attached the token. Pop it off the queue.
  auto& queue = this->Internals->GetQueue(lock);
  if (!queue.empty() && queue.front() == token)
  {
    queue.pop_front();
  }
}

template <typename T, typename S>
void ArrayHandle<T, S>::WaitToWrite(LockType& lock, vtkm::cont::Token& token, bool fakeRead) const
{
  this->Enqueue(lock, token);

  // Note that if you deadlocked here, that means that you are trying to do a write operation on an
  // array where an object is reading or writing to it.
  this->Internals->ConditionVariable.wait(
    lock, [&lock, &token, this] { return this->CanWrite(lock, token); });

  if (!fakeRead)
  {
    token.Attach(this->Internals,
                 this->Internals->GetWriteCount(lock),
                 lock,
                 &this->Internals->ConditionVariable);
  }
  else
  {
    // A current feature limitation of ArrayHandle is that it can only exist on one device at
    // a time. Thus, if a read request comes in for a different device, the prepare has to
    // get satisfy a write lock to boot the array off the existing device. However, we don't
    // want to attach the Token as a write lock because the resulting state is for reading only
    // and others might also want to read. So, we have to pretend that this is a read lock even
    // though we have to make a change to the array.
    //
    // The main point is, this condition is a hack that should go away once ArrayHandle supports
    // multiple devices at once.
    token.Attach(this->Internals,
                 this->Internals->GetReadCount(lock),
                 lock,
                 &this->Internals->ConditionVariable);
  }

  // We successfully attached the token. Pop it off the queue.
  auto& queue = this->Internals->GetQueue(lock);
  if (!queue.empty() && queue.front() == token)
  {
    queue.pop_front();
  }
}

template <typename T, typename S>
void ArrayHandle<T, S>::Enqueue(const vtkm::cont::Token& token) const
{
  LockType lock = this->GetLock();
  this->Enqueue(lock, token);
}

template <typename T, typename S>
void ArrayHandle<T, S>::Enqueue(const LockType& lock, const vtkm::cont::Token& token) const
{
  if (token.IsAttached(this->Internals->GetWriteCount(lock)) ||
      token.IsAttached(this->Internals->GetReadCount(lock)))
  {
    // Do not need to enqueue if we are already attached.
    return;
  }

  auto& queue = this->Internals->GetQueue(lock);
  if (std::find(queue.begin(), queue.end(), token.GetReference()) != queue.end())
  {
    // This token is already in the queue.
    return;
  }

  this->Internals->GetQueue(lock).push_back(token.GetReference());
}
}
} // vtkm::cont

#endif //vtk_m_cont_ArrayHandle_hxx
