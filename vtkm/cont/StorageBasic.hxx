//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/StorageBasic.h>

#include <limits>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>::Storage()
  : Array(nullptr)
  , NumberOfValues(0)
  , AllocatedSize(0)
  , DeallocateOnRelease(true)
{
}

template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>::Storage(const T* array, vtkm::Id numberOfValues)
  : Array(const_cast<T*>(array))
  , NumberOfValues(numberOfValues)
  , AllocatedSize(numberOfValues)
  , DeallocateOnRelease(array == nullptr ? true : false)
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
  , DeallocateOnRelease(src.DeallocateOnRelease)
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
  if (numberOfValues < 0)
  {
    throw vtkm::cont::ErrorBadAllocation("Cannot allocate an array with negative size.");
  }

  // Check that the number of bytes won't be more than a size_t can hold.
  const size_t maxNumValues = std::numeric_limits<size_t>::max() / sizeof(T);
  if (static_cast<vtkm::UInt64>(numberOfValues) > static_cast<vtkm::UInt64>(maxNumValues))
  {
    throw ErrorBadAllocation("Requested allocation exceeds size_t capacity.");
  }
  this->AllocateBytes(static_cast<vtkm::UInt64>(numberOfValues) *
                      static_cast<vtkm::UInt64>(sizeof(T)));
}

template <typename T>
void Storage<T, vtkm::cont::StorageTagBasic>::AllocateBytes(vtkm::UInt64 numberOfBytes)
{
  const vtkm::Id numberOfValues =
    static_cast<vtkm::Id>(numberOfBytes / static_cast<vtkm::UInt64>(sizeof(T)));

  // If we are allocating less data, just shrink the array.
  // (If allocation empty, drop down so we can deallocate memory.)
  if ((numberOfValues <= this->AllocatedSize) && (numberOfValues > 0))
  {
    this->NumberOfValues = numberOfValues;
    return;
  }

  if (!this->DeallocateOnRelease)
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
}

template <typename T>
void Storage<T, vtkm::cont::StorageTagBasic>::Shrink(vtkm::Id numberOfValues)
{
  this->ShrinkBytes(static_cast<vtkm::UInt64>(numberOfValues) *
                    static_cast<vtkm::UInt64>(sizeof(T)));
}

template <typename T>
void Storage<T, vtkm::cont::StorageTagBasic>::ShrinkBytes(vtkm::UInt64 numberOfBytes)
{
  if (numberOfBytes > this->GetNumberOfBytes())
  {
    throw vtkm::cont::ErrorBadValue("Shrink method cannot be used to grow array.");
  }

  this->NumberOfValues =
    static_cast<vtkm::Id>(numberOfBytes / static_cast<vtkm::UInt64>(sizeof(T)));
}

template <typename T>
T* Storage<T, vtkm::cont::StorageTagBasic>::StealArray()
{
  this->DeallocateOnRelease = false;
  return this->Array;
}

} // namespace internal
}
} // namespace vtkm::cont
