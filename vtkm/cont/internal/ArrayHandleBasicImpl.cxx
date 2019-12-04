//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define vtkm_cont_internal_ArrayHandleImpl_cxx

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/internal/ArrayHandleBasicImpl.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

TypelessExecutionArray::TypelessExecutionArray(void*& executionArray,
                                               void*& executionArrayEnd,
                                               void*& executionArrayCapacity,
                                               const StorageBasicBase* controlArray)
  : Array(executionArray)
  , ArrayEnd(executionArrayEnd)
  , ArrayCapacity(executionArrayCapacity)
  , ArrayControl(controlArray->GetBasePointer())
  , ArrayControlCapacity(controlArray->GetCapacityPointer())
{
}

ExecutionArrayInterfaceBasicBase::ExecutionArrayInterfaceBasicBase(StorageBasicBase& storage)
  : ControlStorage(storage)
{
}

ExecutionArrayInterfaceBasicBase::~ExecutionArrayInterfaceBasicBase()
{
}

ArrayHandleImpl::InternalStruct::~InternalStruct()
{
  LockType lock(this->Mutex);
  if (this->ExecutionArrayValid && this->ExecutionInterface != nullptr &&
      this->ExecutionArray != nullptr)
  {
    TypelessExecutionArray execArray = this->MakeTypelessExecutionArray(lock);
    this->ExecutionInterface->Free(execArray);
  }

  delete this->ControlArray;
  delete this->ExecutionInterface;
}

TypelessExecutionArray ArrayHandleImpl::InternalStruct::MakeTypelessExecutionArray(
  const LockType& lock)
{
  return TypelessExecutionArray(this->GetExecutionArray(lock),
                                this->GetExecutionArrayEnd(lock),
                                this->GetExecutionArrayCapacity(lock),
                                this->GetControlArray(lock));
}

void ArrayHandleImpl::CheckControlArrayValid(const LockType& lock)
{
  if (!this->Internals->IsControlArrayValid(lock))
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

vtkm::Id ArrayHandleImpl::GetNumberOfValues(const LockType& lock, vtkm::UInt64 sizeOfT) const
{
  if (this->Internals->IsControlArrayValid(lock))
  {
    return this->Internals->GetControlArray(lock)->GetNumberOfValues();
  }
  else if (this->Internals->IsExecutionArrayValid(lock))
  {
    auto numBytes = static_cast<char*>(this->Internals->GetExecutionArrayEnd(lock)) -
      static_cast<char*>(this->Internals->GetExecutionArray(lock));
    return static_cast<vtkm::Id>(numBytes) / static_cast<vtkm::Id>(sizeOfT);
  }
  else
  {
    return 0;
  }
}

void ArrayHandleImpl::Allocate(const LockType& lock, vtkm::Id numberOfValues, vtkm::UInt64 sizeOfT)
{
  this->ReleaseResourcesExecutionInternal(lock);
  this->Internals->GetControlArray(lock)->AllocateValues(numberOfValues, sizeOfT);
  this->Internals->SetControlArrayValid(lock, true);
}

void ArrayHandleImpl::Shrink(const LockType& lock, vtkm::Id numberOfValues, vtkm::UInt64 sizeOfT)
{
  VTKM_ASSERT(numberOfValues >= 0);

  if (numberOfValues > 0)
  {
    vtkm::Id originalNumberOfValues = this->GetNumberOfValues(lock, sizeOfT);

    if (numberOfValues < originalNumberOfValues)
    {
      if (this->Internals->IsControlArrayValid(lock))
      {
        this->Internals->GetControlArray(lock)->Shrink(numberOfValues);
      }
      if (this->Internals->IsExecutionArrayValid(lock))
      {
        auto offset = static_cast<vtkm::UInt64>(numberOfValues) * sizeOfT;
        this->Internals->GetExecutionArrayEnd(lock) =
          static_cast<char*>(this->Internals->GetExecutionArray(lock)) + offset;
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

    VTKM_ASSERT(this->GetNumberOfValues(lock, sizeOfT) == numberOfValues);
  }
  else // numberOfValues == 0
  {
    // If we are shrinking to 0, there is nothing to save and we might as well
    // free up memory. Plus, some storage classes expect that data will be
    // deallocated when the size goes to zero.
    this->Allocate(lock, 0, sizeOfT);
  }
}

void ArrayHandleImpl::ReleaseResources(const LockType& lock)
{
  this->ReleaseResourcesExecutionInternal(lock);

  if (this->Internals->IsControlArrayValid(lock))
  {
    this->Internals->GetControlArray(lock)->ReleaseResources();
    this->Internals->SetControlArrayValid(lock, false);
  }
}

void ArrayHandleImpl::PrepareForInput(const LockType& lock, vtkm::UInt64 sizeOfT) const
{
  const vtkm::Id numVals = this->GetNumberOfValues(lock, sizeOfT);
  const vtkm::UInt64 numBytes = sizeOfT * static_cast<vtkm::UInt64>(numVals);
  if (!this->Internals->IsExecutionArrayValid(lock))
  {
    // Initialize an empty array if needed:
    if (!this->Internals->IsControlArrayValid(lock))
    {
      this->Internals->GetControlArray(lock)->AllocateValues(0, sizeOfT);
      this->Internals->SetControlArrayValid(lock, true);
    }

    TypelessExecutionArray execArray = this->Internals->MakeTypelessExecutionArray(lock);

    this->Internals->GetExecutionInterface(lock)->Allocate(execArray, numVals, sizeOfT);

    this->Internals->GetExecutionInterface(lock)->CopyFromControl(
      this->Internals->GetControlArray(lock)->GetBasePointer(),
      this->Internals->GetExecutionArray(lock),
      numBytes);

    this->Internals->SetExecutionArrayValid(lock, true);
  }
  this->Internals->GetExecutionInterface(lock)->UsingForRead(
    this->Internals->GetControlArray(lock)->GetBasePointer(),
    this->Internals->GetExecutionArray(lock),
    numBytes);
}

void ArrayHandleImpl::PrepareForOutput(const LockType& lock, vtkm::Id numVals, vtkm::UInt64 sizeOfT)
{
  // Invalidate control arrays since we expect the execution data to be
  // overwritten. Don't free control resources in case they're shared with
  // the execution environment.
  this->Internals->SetControlArrayValid(lock, false);

  TypelessExecutionArray execArray = this->Internals->MakeTypelessExecutionArray(lock);

  this->Internals->GetExecutionInterface(lock)->Allocate(execArray, numVals, sizeOfT);
  const vtkm::UInt64 numBytes = sizeOfT * static_cast<vtkm::UInt64>(numVals);
  this->Internals->GetExecutionInterface(lock)->UsingForWrite(
    this->Internals->GetControlArray(lock)->GetBasePointer(),
    this->Internals->GetExecutionArray(lock),
    numBytes);

  this->Internals->SetExecutionArrayValid(lock, true);
}

void ArrayHandleImpl::PrepareForInPlace(const LockType& lock, vtkm::UInt64 sizeOfT)
{
  const vtkm::Id numVals = this->GetNumberOfValues(lock, sizeOfT);
  const vtkm::UInt64 numBytes = sizeOfT * static_cast<vtkm::UInt64>(numVals);

  if (!this->Internals->IsExecutionArrayValid(lock))
  {
    // Initialize an empty array if needed:
    if (!this->Internals->IsControlArrayValid(lock))
    {
      this->Internals->GetControlArray(lock)->AllocateValues(0, sizeOfT);
      this->Internals->SetControlArrayValid(lock, true);
    }

    TypelessExecutionArray execArray = this->Internals->MakeTypelessExecutionArray(lock);

    this->Internals->GetExecutionInterface(lock)->Allocate(execArray, numVals, sizeOfT);

    this->Internals->GetExecutionInterface(lock)->CopyFromControl(
      this->Internals->GetControlArray(lock)->GetBasePointer(),
      this->Internals->GetExecutionArray(lock),
      numBytes);

    this->Internals->SetExecutionArrayValid(lock, true);
  }

  this->Internals->GetExecutionInterface(lock)->UsingForReadWrite(
    this->Internals->GetControlArray(lock)->GetBasePointer(),
    this->Internals->GetExecutionArray(lock),
    numBytes);

  // Invalidate the control array, since we expect the values to be modified:
  this->Internals->SetControlArrayValid(lock, false);
}

bool ArrayHandleImpl::PrepareForDevice(const LockType& lock,
                                       DeviceAdapterId devId,
                                       vtkm::UInt64 sizeOfT) const
{
  // Check if the current device matches the last one and sync through
  // the control environment if the device changes.
  if (this->Internals->GetExecutionInterface(lock))
  {
    if (this->Internals->GetExecutionInterface(lock)->GetDeviceId() == devId)
    {
      // All set, nothing to do.
      return false;
    }
    else
    {
      // Update the device allocator:
      this->SyncControlArray(lock, sizeOfT);
      TypelessExecutionArray execArray = this->Internals->MakeTypelessExecutionArray(lock);
      this->Internals->GetExecutionInterface(lock)->Free(execArray);
      this->Internals->SetExecutionArrayValid(lock, false);
      return true;
    }
  }

  VTKM_ASSERT(!this->Internals->IsExecutionArrayValid(lock));
  return true;
}

DeviceAdapterId ArrayHandleImpl::GetDeviceAdapterId(const LockType& lock) const
{
  return this->Internals->IsExecutionArrayValid(lock)
    ? this->Internals->GetExecutionInterface(lock)->GetDeviceId()
    : DeviceAdapterTagUndefined{};
}


void ArrayHandleImpl::SyncControlArray(const LockType& lock, vtkm::UInt64 sizeOfT) const
{
  if (!this->Internals->IsControlArrayValid(lock))
  {
    // Need to change some state that does not change the logical state from
    // an external point of view.
    if (this->Internals->IsExecutionArrayValid(lock))
    {
      const vtkm::UInt64 numBytes =
        static_cast<vtkm::UInt64>(static_cast<char*>(this->Internals->GetExecutionArrayEnd(lock)) -
                                  static_cast<char*>(this->Internals->GetExecutionArray(lock)));
      const vtkm::Id numVals = static_cast<vtkm::Id>(numBytes / sizeOfT);

      this->Internals->GetControlArray(lock)->AllocateValues(numVals, sizeOfT);
      this->Internals->GetExecutionInterface(lock)->CopyToControl(
        this->Internals->GetExecutionArray(lock),
        this->Internals->GetControlArray(lock)->GetBasePointer(),
        numBytes);
      this->Internals->SetControlArrayValid(lock, true);
    }
    else
    {
      // This array is in the null state (there is nothing allocated), but
      // the calling function wants to do something with the array. Put this
      // class into a valid state by allocating an array of size 0.
      this->Internals->GetControlArray(lock)->AllocateValues(0, sizeOfT);
      this->Internals->SetControlArrayValid(lock, true);
    }
  }
}

void ArrayHandleImpl::ReleaseResourcesExecutionInternal(const LockType& lock)
{
  if (this->Internals->IsExecutionArrayValid(lock))
  {
    TypelessExecutionArray execArray = this->Internals->MakeTypelessExecutionArray(lock);
    this->Internals->GetExecutionInterface(lock)->Free(execArray);
    this->Internals->SetExecutionArrayValid(lock, false);
  }
}

} // end namespace internal
}
} // end vtkm::cont

#ifdef VTKM_MSVC
//Export this when being used with std::shared_ptr
template class VTKM_CONT_EXPORT std::shared_ptr<vtkm::cont::internal::ArrayHandleImpl>;
#endif
