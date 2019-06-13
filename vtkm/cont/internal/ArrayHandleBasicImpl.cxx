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

TypelessExecutionArray::TypelessExecutionArray(const ArrayHandleImpl* data)
  : Array(data->ExecutionArray)
  , ArrayEnd(data->ExecutionArrayEnd)
  , ArrayCapacity(data->ExecutionArrayCapacity)
  , ArrayControl(data->ControlArray->GetBasePointer())
  , ArrayControlCapacity(data->ControlArray->GetCapacityPointer())
{
}

ExecutionArrayInterfaceBasicBase::ExecutionArrayInterfaceBasicBase(StorageBasicBase& storage)
  : ControlStorage(storage)
{
}

ExecutionArrayInterfaceBasicBase::~ExecutionArrayInterfaceBasicBase()
{
}

ArrayHandleImpl::~ArrayHandleImpl()
{
  if (this->ExecutionArrayValid && this->ExecutionInterface != nullptr &&
      this->ExecutionArray != nullptr)
  {
    TypelessExecutionArray execArray(this);
    this->ExecutionInterface->Free(execArray);
  }

  delete this->ControlArray;
  delete this->ExecutionInterface;
}

void ArrayHandleImpl::CheckControlArrayValid()
{
  if (!this->ControlArrayValid)
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandle::SyncControlArray did not make control array valid.");
  }
}

vtkm::Id ArrayHandleImpl::GetNumberOfValues(vtkm::UInt64 sizeOfT) const
{
  if (this->ControlArrayValid)
  {
    return this->ControlArray->GetNumberOfValues();
  }
  else if (this->ExecutionArrayValid)
  {
    auto numBytes =
      static_cast<char*>(this->ExecutionArrayEnd) - static_cast<char*>(this->ExecutionArray);
    return static_cast<vtkm::Id>(numBytes) / static_cast<vtkm::Id>(sizeOfT);
  }
  else
  {
    return 0;
  }
}

void ArrayHandleImpl::Allocate(vtkm::Id numberOfValues, vtkm::UInt64 sizeOfT)
{
  this->ReleaseResourcesExecutionInternal();
  this->ControlArray->AllocateValues(numberOfValues, sizeOfT);
  this->ControlArrayValid = true;
}

void ArrayHandleImpl::Shrink(vtkm::Id numberOfValues, vtkm::UInt64 sizeOfT)
{
  VTKM_ASSERT(numberOfValues >= 0);

  if (numberOfValues > 0)
  {
    vtkm::Id originalNumberOfValues = this->GetNumberOfValues(sizeOfT);

    if (numberOfValues < originalNumberOfValues)
    {
      if (this->ControlArrayValid)
      {
        this->ControlArray->Shrink(numberOfValues);
      }
      if (this->ExecutionArrayValid)
      {
        auto offset = static_cast<vtkm::UInt64>(numberOfValues) * sizeOfT;
        this->ExecutionArrayEnd = static_cast<char*>(this->ExecutionArray) + offset;
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

    VTKM_ASSERT(this->GetNumberOfValues(sizeOfT) == numberOfValues);
  }
  else // numberOfValues == 0
  {
    // If we are shrinking to 0, there is nothing to save and we might as well
    // free up memory. Plus, some storage classes expect that data will be
    // deallocated when the size goes to zero.
    this->Allocate(0, sizeOfT);
  }
}

void ArrayHandleImpl::ReleaseResources()
{
  this->ReleaseResourcesExecutionInternal();

  if (this->ControlArrayValid)
  {
    this->ControlArray->ReleaseResources();
    this->ControlArrayValid = false;
  }
}

void ArrayHandleImpl::PrepareForInput(vtkm::UInt64 sizeOfT) const
{
  const vtkm::Id numVals = this->GetNumberOfValues(sizeOfT);
  const vtkm::UInt64 numBytes = sizeOfT * static_cast<vtkm::UInt64>(numVals);
  if (!this->ExecutionArrayValid)
  {
    // Initialize an empty array if needed:
    if (!this->ControlArrayValid)
    {
      this->ControlArray->AllocateValues(0, sizeOfT);
      this->ControlArrayValid = true;
    }

    TypelessExecutionArray execArray(this);

    this->ExecutionInterface->Allocate(execArray, numVals, sizeOfT);

    this->ExecutionInterface->CopyFromControl(
      this->ControlArray->GetBasePointer(), this->ExecutionArray, numBytes);

    this->ExecutionArrayValid = true;
  }
  this->ExecutionInterface->UsingForRead(
    this->ControlArray->GetBasePointer(), this->ExecutionArray, numBytes);
}

void ArrayHandleImpl::PrepareForOutput(vtkm::Id numVals, vtkm::UInt64 sizeOfT)
{
  // Invalidate control arrays since we expect the execution data to be
  // overwritten. Don't free control resources in case they're shared with
  // the execution environment.
  this->ControlArrayValid = false;

  TypelessExecutionArray execArray(this);

  this->ExecutionInterface->Allocate(execArray, numVals, sizeOfT);
  const vtkm::UInt64 numBytes = sizeOfT * static_cast<vtkm::UInt64>(numVals);
  this->ExecutionInterface->UsingForWrite(
    this->ControlArray->GetBasePointer(), this->ExecutionArray, numBytes);

  this->ExecutionArrayValid = true;
}

void ArrayHandleImpl::PrepareForInPlace(vtkm::UInt64 sizeOfT)
{
  const vtkm::Id numVals = this->GetNumberOfValues(sizeOfT);
  const vtkm::UInt64 numBytes = sizeOfT * static_cast<vtkm::UInt64>(numVals);

  if (!this->ExecutionArrayValid)
  {
    // Initialize an empty array if needed:
    if (!this->ControlArrayValid)
    {
      this->ControlArray->AllocateValues(0, sizeOfT);
      this->ControlArrayValid = true;
    }

    TypelessExecutionArray execArray(this);

    this->ExecutionInterface->Allocate(execArray, numVals, sizeOfT);

    this->ExecutionInterface->CopyFromControl(
      this->ControlArray->GetBasePointer(), this->ExecutionArray, numBytes);

    this->ExecutionArrayValid = true;
  }

  this->ExecutionInterface->UsingForReadWrite(
    this->ControlArray->GetBasePointer(), this->ExecutionArray, numBytes);

  // Invalidate the control array, since we expect the values to be modified:
  this->ControlArrayValid = false;
}

bool ArrayHandleImpl::PrepareForDevice(DeviceAdapterId devId, vtkm::UInt64 sizeOfT) const
{
  // Check if the current device matches the last one and sync through
  // the control environment if the device changes.
  if (this->ExecutionInterface)
  {
    if (this->ExecutionInterface->GetDeviceId() == devId)
    {
      // All set, nothing to do.
      return false;
    }
    else
    {
      // Update the device allocator:
      this->SyncControlArray(sizeOfT);
      TypelessExecutionArray execArray(this);
      this->ExecutionInterface->Free(execArray);
      delete this->ExecutionInterface;
      this->ExecutionInterface = nullptr;
      this->ExecutionArrayValid = false;
    }
  }

  VTKM_ASSERT(this->ExecutionInterface == nullptr);
  VTKM_ASSERT(!this->ExecutionArrayValid);
  return true;
}

DeviceAdapterId ArrayHandleImpl::GetDeviceAdapterId() const
{
  return this->ExecutionArrayValid ? this->ExecutionInterface->GetDeviceId()
                                   : DeviceAdapterTagUndefined{};
}


void ArrayHandleImpl::SyncControlArray(vtkm::UInt64 sizeOfT) const
{
  if (!this->ControlArrayValid)
  {
    // Need to change some state that does not change the logical state from
    // an external point of view.
    if (this->ExecutionArrayValid)
    {
      const vtkm::UInt64 numBytes = static_cast<vtkm::UInt64>(
        static_cast<char*>(this->ExecutionArrayEnd) - static_cast<char*>(this->ExecutionArray));
      const vtkm::Id numVals = static_cast<vtkm::Id>(numBytes / sizeOfT);

      this->ControlArray->AllocateValues(numVals, sizeOfT);
      this->ExecutionInterface->CopyToControl(
        this->ExecutionArray, this->ControlArray->GetBasePointer(), numBytes);
      this->ControlArrayValid = true;
    }
    else
    {
      // This array is in the null state (there is nothing allocated), but
      // the calling function wants to do something with the array. Put this
      // class into a valid state by allocating an array of size 0.
      this->ControlArray->AllocateValues(0, sizeOfT);
      this->ControlArrayValid = true;
    }
  }
}

void ArrayHandleImpl::ReleaseResourcesExecutionInternal()
{
  if (this->ExecutionArrayValid)
  {
    TypelessExecutionArray execArray(this);
    this->ExecutionInterface->Free(execArray);
    this->ExecutionArrayValid = false;
  }
}

} // end namespace internal
}
} // end vtkm::cont

#ifdef VTKM_MSVC
//Export this when being used with std::shared_ptr
template class VTKM_CONT_EXPORT std::shared_ptr<vtkm::cont::internal::ArrayHandleImpl>;
#endif
