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

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>::Storage(const T* array, vtkm::Id numberOfValues)
  : Array(const_cast<T*>(array))
  , NumberOfValues(numberOfValues)
  , AllocatedSize(numberOfValues)
  , DeallocateOnRelease(false)
  , UserProvidedMemory(array == nullptr ? false : true)
{
}

template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>::~Storage()
{
  this->ReleaseResources();
}

template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>::Storage(const Storage<T, StorageTagBasic>& src)
  : Array(src.Array)
  , NumberOfValues(src.NumberOfValues)
  , AllocatedSize(src.AllocatedSize)
  , DeallocateOnRelease(false)
  , UserProvidedMemory(src.UserProvidedMemory)
{
  if (src.DeallocateOnRelease)
  {
    throw vtkm::cont::ErrorBadValue(
      "Attempted to copy a storage array that needs deallocation. "
      "This is disallowed to prevent complications with deallocation.");
  }
}

template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>& Storage<T, vtkm::cont::StorageTagBasic>::operator=(
  const Storage<T, StorageTagBasic>& src)
{
  if (src.DeallocateOnRelease)
  {
    throw vtkm::cont::ErrorBadValue(
      "Attempted to copy a storage array that needs deallocation. "
      "This is disallowed to prevent complications with deallocation.");
  }

  this->ReleaseResources();
  this->Array = src.Array;
  this->NumberOfValues = src.NumberOfValues;
  this->AllocatedSize = src.AllocatedSize;
  this->DeallocateOnRelease = src.DeallocateOnRelease;
  this->UserProvidedMemory = src.UserProvidedMemory;

  return *this;
}

template <typename T>
void Storage<T, vtkm::cont::StorageTagBasic>::ReleaseResources()
{
  if (this->NumberOfValues > 0)
  {
    VTKM_ASSERT(this->Array != nullptr);
    if (this->DeallocateOnRelease)
    {
      AllocatorType allocator;
      allocator.deallocate(this->Array, static_cast<std::size_t>(this->AllocatedSize));
    }
    this->Array = nullptr;
    this->NumberOfValues = 0;
    this->AllocatedSize = 0;
  }
  else
  {
    VTKM_ASSERT(this->Array == nullptr);
  }
}

template <typename T>
void Storage<T, vtkm::cont::StorageTagBasic>::Allocate(vtkm::Id numberOfValues)
{
  // If we are allocating less data, just shrink the array.
  // (If allocation empty, drop down so we can deallocate memory.)
  if ((numberOfValues <= this->AllocatedSize) && (numberOfValues > 0))
  {
    this->NumberOfValues = numberOfValues;
    return;
  }

  if (this->UserProvidedMemory)
  {
    throw vtkm::cont::ErrorBadValue("User allocated arrays cannot be reallocated.");
  }

  this->ReleaseResources();
  try
  {
    if (numberOfValues > 0)
    {
      AllocatorType allocator;
      this->Array = allocator.allocate(static_cast<std::size_t>(numberOfValues));
      this->AllocatedSize = numberOfValues;
      this->NumberOfValues = numberOfValues;
    }
    else
    {
      // ReleaseResources should have already set AllocatedSize to 0.
      VTKM_ASSERT(this->AllocatedSize == 0);
    }
  }
  catch (std::bad_alloc&)
  {
    // Make sureour state is OK.
    this->Array = nullptr;
    this->NumberOfValues = 0;
    this->AllocatedSize = 0;
    throw vtkm::cont::ErrorBadAllocation("Could not allocate basic control array.");
  }

  this->DeallocateOnRelease = true;
  this->UserProvidedMemory = false;
}

template <typename T>
void Storage<T, vtkm::cont::StorageTagBasic>::Shrink(vtkm::Id numberOfValues)
{
  if (numberOfValues > this->GetNumberOfValues())
  {
    throw vtkm::cont::ErrorBadValue("Shrink method cannot be used to grow array.");
  }

  this->NumberOfValues = numberOfValues;
}

template <typename T>
T* Storage<T, vtkm::cont::StorageTagBasic>::StealArray()
{
  T* saveArray = this->Array;
  this->Array = nullptr;
  this->NumberOfValues = 0;
  this->AllocatedSize = 0;
  return saveArray;
}

} // namespace internal
}
} // namespace vtkm::cont
