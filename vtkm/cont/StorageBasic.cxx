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

#define vtkm_cont_StorageBasic_cxx
#include <vtkm/cont/StorageBasic.h>
#include <vtkm/internal/Configure.h>

#if defined(VTKM_POSIX)
#define VTKM_MEMALIGN_POSIX
#elif defined(_WIN32)
#define VTKM_MEMALIGN_WIN
#elif defined(__SSE__)
#define VTKM_MEMALIGN_SSE
#else
#define VTKM_MEMALIGN_NONE
#endif

#if defined(VTKM_MEMALIGN_POSIX)
#include <stdlib.h>
#elif defined(VTKM_MEMALIGN_WIN)
#include <malloc.h>
#elif defined(VTKM_MEMALIGN_SSE)
#include <xmmintrin.h>
#else
#include <malloc.h>
#endif

#include <cstddef>
#include <cstdlib>

namespace vtkm
{
namespace cont
{
namespace internal
{

void free_memory(void* mem)
{
#if defined(VTKM_MEMALIGN_POSIX)
  free(mem);
#elif defined(VTKM_MEMALIGN_WIN)
  _aligned_free(mem);
#elif defined(VTKM_MEMALIGN_SSE)
  _mm_free(mem);
#else
  free(mem);
#endif
}

void* StorageBasicAllocator::allocate(size_t size, size_t align)
{
#if defined(VTKM_MEMALIGN_POSIX)
  void* mem = nullptr;
  if (posix_memalign(&mem, align, size) != 0)
  {
    mem = nullptr;
  }
#elif defined(VTKM_MEMALIGN_WIN)
  void* mem = _aligned_malloc(size, align);
#elif defined(VTKM_MEMALIGN_SSE)
  void* mem = _mm_malloc(size, align);
#else
  void* mem = malloc(size);
#endif
  return mem;
}

StorageBasicBase::StorageBasicBase()
  : Array(nullptr)
  , AllocatedByteSize(0)
  , NumberOfValues(0)
  , DeleteFunction(internal::free_memory)
{
}

StorageBasicBase::StorageBasicBase(const void* array,
                                   vtkm::Id numberOfValues,
                                   vtkm::UInt64 sizeOfValue)
  : Array(const_cast<void*>(array))
  , AllocatedByteSize(static_cast<vtkm::UInt64>(numberOfValues) * sizeOfValue)
  , NumberOfValues(numberOfValues)
  , DeleteFunction(array == nullptr ? internal::free_memory : nullptr)
{
}

StorageBasicBase::StorageBasicBase(const void* array,
                                   vtkm::Id numberOfValues,
                                   vtkm::UInt64 sizeOfValue,
                                   void (*deleteFunction)(void*))
  : Array(const_cast<void*>(array))
  , AllocatedByteSize(static_cast<vtkm::UInt64>(numberOfValues) * sizeOfValue)
  , NumberOfValues(numberOfValues)
  , DeleteFunction(deleteFunction)
{
}

StorageBasicBase::~StorageBasicBase()
{
  this->ReleaseResources();
}

StorageBasicBase::StorageBasicBase(StorageBasicBase&& src)
  : Array(src.Array)
  , AllocatedByteSize(src.AllocatedByteSize)
  , NumberOfValues(src.NumberOfValues)
  , DeleteFunction(src.DeleteFunction)

{
  src.Array = nullptr;
  src.AllocatedByteSize = 0;
  src.NumberOfValues = 0;
  src.DeleteFunction = nullptr;
}

StorageBasicBase::StorageBasicBase(const StorageBasicBase& src)
  : Array(src.Array)
  , AllocatedByteSize(src.AllocatedByteSize)
  , NumberOfValues(src.NumberOfValues)
  , DeleteFunction(src.DeleteFunction)

{
  if (src.DeleteFunction)
  {
    throw vtkm::cont::ErrorBadValue(
      "Attempted to copy a storage array that needs deallocation. "
      "This is disallowed to prevent complications with deallocation.");
  }
}

StorageBasicBase StorageBasicBase::operator=(StorageBasicBase&& src)
{
  this->ReleaseResources();
  this->Array = src.Array;
  this->AllocatedByteSize = src.AllocatedByteSize;
  this->NumberOfValues = src.NumberOfValues;
  this->DeleteFunction = src.DeleteFunction;

  src.Array = nullptr;
  src.AllocatedByteSize = 0;
  src.NumberOfValues = 0;
  src.DeleteFunction = nullptr;
  return *this;
}

StorageBasicBase StorageBasicBase::operator=(const StorageBasicBase& src)
{
  if (src.DeleteFunction)
  {
    throw vtkm::cont::ErrorBadValue(
      "Attempted to copy a storage array that needs deallocation. "
      "This is disallowed to prevent complications with deallocation.");
  }

  this->ReleaseResources();
  this->Array = src.Array;
  this->AllocatedByteSize = src.AllocatedByteSize;
  this->NumberOfValues = src.NumberOfValues;
  this->DeleteFunction = src.DeleteFunction;
  return *this;
}

void StorageBasicBase::AllocateValues(vtkm::Id numberOfValues, vtkm::UInt64 sizeOfValue)
{
  if (numberOfValues < 0)
  {
    throw vtkm::cont::ErrorBadAllocation("Cannot allocate an array with negative size.");
  }

  // Check that the number of bytes won't be more than a size_t can hold.
  const size_t maxNumValues = std::numeric_limits<size_t>::max() / sizeOfValue;
  if (static_cast<vtkm::UInt64>(numberOfValues) > maxNumValues)
  {
    throw ErrorBadAllocation("Requested allocation exceeds size_t capacity.");
  }

  // If we are allocating less data, just shrink the array.
  // (If allocation empty, drop down so we can deallocate memory.)
  vtkm::UInt64 allocsize = static_cast<vtkm::UInt64>(numberOfValues) * sizeOfValue;
  if ((allocsize <= this->AllocatedByteSize) && (numberOfValues > 0))
  {
    this->NumberOfValues = numberOfValues;
    return;
  }

  if (!this->DeleteFunction)
  {
    throw vtkm::cont::ErrorBadValue("User allocated arrays cannot be reallocated.");
  }

  this->ReleaseResources();

  if (numberOfValues > 0)
  {
    this->Array = AllocatorType{}.allocate(allocsize, VTKM_ALLOCATION_ALIGNMENT);
    this->AllocatedByteSize = allocsize;
    this->NumberOfValues = numberOfValues;
    this->DeleteFunction = internal::free_memory;
    if (this->Array == nullptr)
    {
      // Make sure our state is OK.
      this->AllocatedByteSize = 0;
      this->NumberOfValues = 0;
      throw vtkm::cont::ErrorBadAllocation("Could not allocate basic control array.");
    }
  }
  else
  {
    // ReleaseResources should have already set NumberOfValues to 0.
    VTKM_ASSERT(this->NumberOfValues == 0);
    VTKM_ASSERT(this->AllocatedByteSize == 0);
  }
}

void StorageBasicBase::Shrink(vtkm::Id numberOfValues)
{
  if (numberOfValues > this->NumberOfValues)
  {
    throw vtkm::cont::ErrorBadValue("Shrink method cannot be used to grow array.");
  }

  this->NumberOfValues = numberOfValues;
}

void StorageBasicBase::ReleaseResources()
{
  if (this->AllocatedByteSize > 0)
  {
    VTKM_ASSERT(this->Array != nullptr);
    if (this->DeleteFunction)
    {
      this->DeleteFunction(this->Array);
    }
    this->Array = nullptr;
    this->AllocatedByteSize = 0;
    this->NumberOfValues = 0;
  }
  else
  {
    VTKM_ASSERT(this->Array == nullptr);
  }
}

void StorageBasicBase::SetBasePointer(const void* ptr,
                                      vtkm::Id numberOfValues,
                                      vtkm::UInt64 sizeOfValue,
                                      void (*deleteFunction)(void*))
{
  this->ReleaseResources();
  this->Array = const_cast<void*>(ptr);
  this->AllocatedByteSize = static_cast<vtkm::UInt64>(numberOfValues) * sizeOfValue;
  this->NumberOfValues = numberOfValues;
  this->DeleteFunction = deleteFunction;
}

void* StorageBasicBase::GetBasePointer() const
{
  return this->Array;
}

void* StorageBasicBase::GetEndPointer(vtkm::Id numberOfValues, vtkm::UInt64 sizeOfValue) const
{
  VTKM_ASSERT(this->NumberOfValues == numberOfValues);
  if (!this->Array)
  {
    return nullptr;
  }

  auto p = static_cast<vtkm::UInt8*>(this->Array);
  auto offset = static_cast<vtkm::UInt64>(this->NumberOfValues) * sizeOfValue;
  return static_cast<void*>(p + offset);
}

void* StorageBasicBase::GetCapacityPointer() const
{
  if (!this->Array)
  {
    return nullptr;
  }
  auto v = static_cast<vtkm::UInt8*>(this->Array) + AllocatedByteSize;
  return static_cast<void*>(v);
}

#define _VTKM_STORAGE_INSTANTIATE(Type)                                                            \
  template class VTKM_CONT_EXPORT Storage<Type, StorageTagBasic>;                                  \
  template class VTKM_CONT_EXPORT Storage<vtkm::Vec<Type, 2>, StorageTagBasic>;                    \
  template class VTKM_CONT_EXPORT Storage<vtkm::Vec<Type, 3>, StorageTagBasic>;                    \
  template class VTKM_CONT_EXPORT Storage<vtkm::Vec<Type, 4>, StorageTagBasic>;

_VTKM_STORAGE_INSTANTIATE(char)
_VTKM_STORAGE_INSTANTIATE(vtkm::Int8)
_VTKM_STORAGE_INSTANTIATE(vtkm::UInt8)
_VTKM_STORAGE_INSTANTIATE(vtkm::Int16)
_VTKM_STORAGE_INSTANTIATE(vtkm::UInt16)
_VTKM_STORAGE_INSTANTIATE(vtkm::Int32)
_VTKM_STORAGE_INSTANTIATE(vtkm::UInt32)
_VTKM_STORAGE_INSTANTIATE(vtkm::Int64)
_VTKM_STORAGE_INSTANTIATE(vtkm::UInt64)
_VTKM_STORAGE_INSTANTIATE(vtkm::Float32)
_VTKM_STORAGE_INSTANTIATE(vtkm::Float64)
}
}
} // namespace vtkm::cont::internal
