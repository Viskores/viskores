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
#ifndef vtk_m_cont_StorageBasic_h
#define vtk_m_cont_StorageBasic_h

#include <vtkm/Assert.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/cont/ErrorControlBadAllocation.h>
#include <vtkm/cont/Storage.h>

#include <vtkm/cont/internal/ArrayPortalFromIterators.h>

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

// Defines the cache line size in bytes to align allocations to
#define VTKM_CACHE_LINE_SIZE 64

namespace vtkm {
namespace cont {

/// A tag for the basic implementation of a Storage object.
struct StorageTagBasic {  };

namespace internal {

inline void* alloc_aligned(size_t size, size_t align){
#if defined(VTKM_MEMALIGN_POSIX)
  void *mem = NULL;
  if (posix_memalign(&mem, align, size) != 0){
    mem = NULL;
  }
#elif defined(VTKM_MEMALIGN_WIN)
  void *mem = _aligned_malloc(size, align);
#elif defined(VTKM_MEMALIGN_SSE)
  void *mem = _mm_malloc(size, align);
#else
  void *mem = malloc(size);
#endif
  if (mem == NULL){
    throw std::bad_alloc();
  }
  return mem;
}
inline void free_aligned(void *mem){
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

/// A simple aligned allocator type that will align allocations to `Alignment` bytes
/// TODO: Once C++11 std::allocator_traits is better used by STL and we want to drop
/// support for pre-C++11 we can drop a lot of the typedefs and functions here.
template<typename T, size_t Alignment>
struct AlignedAllocator {
  typedef T value_type;
  typedef T& reference;
  typedef const T& const_reference;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef void* void_pointer;
  typedef const void* const_void_pointer;
  typedef ptrdiff_t difference_type;
  typedef size_t size_type;

  template<typename U>
  struct rebind {
    typedef AlignedAllocator<U, Alignment> other;
  };

  AlignedAllocator(){}

  template<typename Tb>
  AlignedAllocator(const AlignedAllocator<Tb, Alignment>&){}

  pointer allocate(size_t n){
    return static_cast<pointer>(alloc_aligned(n * sizeof(T), Alignment));
  }
  void deallocate(pointer p, size_t){
    free_aligned(static_cast<void*>(p));
  }
  pointer address(reference r){
    return &r;
  }
  const_pointer address(const_reference r){
    return &r;
  }
  size_type max_size() const {
    return std::numeric_limits<size_type>::max() / sizeof(T);
  }
  void construct(pointer p, const T &t){
    new(p) T(t);
  }
  void destroy(pointer p){
    p->~T();
  }
};

template<typename T, typename U, size_t AlignA, size_t AlignB>
bool operator==(const AlignedAllocator<T, AlignA>&, const AlignedAllocator<U, AlignB>&){
  return AlignA == AlignB;
}
template<typename T, typename U, size_t AlignA, size_t AlignB>
bool operator!=(const AlignedAllocator<T, AlignA>&, const AlignedAllocator<U, AlignB>&){
  return AlignA != AlignB;
}

/// A basic implementation of an Storage object.
///
/// \todo This storage does \em not construct the values within the array.
/// Thus, it is important to not use this class with any type that will fail if
/// not constructed. These are things like basic types (int, float, etc.) and
/// the VTKm Tuple classes.  In the future it would be nice to have a compile
/// time check to enforce this.
///
template <typename ValueT>
class Storage<ValueT, vtkm::cont::StorageTagBasic>
{
public:
  typedef ValueT ValueType;
  typedef vtkm::cont::internal::ArrayPortalFromIterators<ValueType*> PortalType;
  typedef vtkm::cont::internal::ArrayPortalFromIterators<const ValueType*> PortalConstType;

  /// The original design of this class provided an allocator as a template
  /// parameters. That messed things up, though, because other templated
  /// classes assume that the \c Storage has one template parameter. There are
  /// other ways to allow you to specify the allocator, but it is uncertain
  /// whether that would ever be useful. So, instead of jumping through hoops
  /// implementing them, just fix the allocator for now.
  ///
  typedef AlignedAllocator<ValueType, VTKM_CACHE_LINE_SIZE> AllocatorType;

public:

  VTKM_CONT_EXPORT
  Storage(const ValueType *array = NULL, vtkm::Id numberOfValues = 0)
    : Array(const_cast<ValueType *>(array)),
      NumberOfValues(numberOfValues),
      AllocatedSize(numberOfValues),
      DeallocateOnRelease(false),
      UserProvidedMemory( array == NULL ? false : true)
  {
  }

  VTKM_CONT_EXPORT
  ~Storage()
  {
    this->ReleaseResources();
  }

  VTKM_CONT_EXPORT
  Storage(const Storage<ValueType, StorageTagBasic> &src)
    : Array(src.Array),
      NumberOfValues(src.NumberOfValues),
      AllocatedSize(src.AllocatedSize),
      DeallocateOnRelease(false),
      UserProvidedMemory(src.UserProvidedMemory)
  {
    if (src.DeallocateOnRelease)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "Attempted to copy a storage array that needs deallocation. "
            "This is disallowed to prevent complications with deallocation.");
    }
  }

  VTKM_CONT_EXPORT
  Storage &operator=(const Storage<ValueType, StorageTagBasic> &src)
  {
    if (src.DeallocateOnRelease)
    {
      throw vtkm::cont::ErrorControlBadValue(
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

  VTKM_CONT_EXPORT
  void ReleaseResources()
  {
    if (this->NumberOfValues > 0)
    {
      VTKM_ASSERT(this->Array != NULL);
      if (this->DeallocateOnRelease)
      {
        AllocatorType allocator;
        allocator.deallocate(this->Array,
                             static_cast<std::size_t>(this->AllocatedSize) );
      }
      this->Array = NULL;
      this->NumberOfValues = 0;
      this->AllocatedSize = 0;
    }
    else
    {
      VTKM_ASSERT(this->Array == NULL);
    }
  }

  VTKM_CONT_EXPORT
  void Allocate(vtkm::Id numberOfValues)
  {
    if (numberOfValues <= this->AllocatedSize)
    {
      this->NumberOfValues = numberOfValues;
      return;
    }

    if(this->UserProvidedMemory)
    {
      throw vtkm::cont::ErrorControlBadValue(
        "User allocated arrays cannot be reallocated.");
    }

    this->ReleaseResources();
    try
    {
      if (numberOfValues > 0)
      {
        AllocatorType allocator;
        this->Array = allocator.allocate(static_cast<std::size_t>(numberOfValues));
        this->AllocatedSize  = numberOfValues;
        this->NumberOfValues = numberOfValues;
      }
      else
      {
        // ReleaseResources should have already set AllocatedSize to 0.
        VTKM_ASSERT(this->AllocatedSize == 0);
      }
    }
    catch (std::bad_alloc err)
    {
      // Make sureour state is OK.
      this->Array = NULL;
      this->NumberOfValues = 0;
      this->AllocatedSize = 0;
      throw vtkm::cont::ErrorControlBadAllocation(
        "Could not allocate basic control array.");
    }

    this->DeallocateOnRelease = true;
    this->UserProvidedMemory = false;
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    return this->NumberOfValues;
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id numberOfValues)
  {
    if (numberOfValues > this->GetNumberOfValues())
    {
      throw vtkm::cont::ErrorControlBadValue(
        "Shrink method cannot be used to grow array.");
    }

    this->NumberOfValues = numberOfValues;
  }

  VTKM_CONT_EXPORT
  PortalType GetPortal()
  {
    return PortalType(this->Array, this->Array + this->NumberOfValues);
  }

  VTKM_CONT_EXPORT
  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->Array, this->Array + this->NumberOfValues);
  }

  /// \brief Take the reference away from this object.
  ///
  /// This method returns the pointer to the array held by this array. It then
  /// clears the internal array pointer to NULL, thereby ensuring that the
  /// Storage will never deallocate the array. This is helpful for taking a
  /// reference for an array created internally by VTK-m and not having to keep
  /// a VTK-m object around. Obviously the caller becomes responsible for
  /// destroying the memory.
  ///
  VTKM_CONT_EXPORT
  ValueType *StealArray()
  {
    ValueType *saveArray =  this->Array;
    this->Array = NULL;
    this->NumberOfValues = 0;
    this->AllocatedSize = 0;
    return saveArray;
  }

private:
  ValueType *Array;
  vtkm::Id NumberOfValues;
  vtkm::Id AllocatedSize;
  bool DeallocateOnRelease;
  bool UserProvidedMemory;
};

} // namespace internal

}
} // namespace vtkm::cont

#endif //vtk_m_cont_StorageBasic_h
