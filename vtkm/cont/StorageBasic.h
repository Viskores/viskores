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

// Defines the cache line size in bytes to align allocations to
#ifndef VTKM_CACHE_LINE_SIZE
#define VTKM_CACHE_LINE_SIZE 64
#endif

namespace vtkm {
namespace cont {

/// A tag for the basic implementation of a Storage object.
struct VTKM_ALWAYS_EXPORT StorageTagBasic {  };

namespace internal {

VTKM_CONT_EXPORT
void* alloc_aligned(size_t size, size_t align);

VTKM_CONT_EXPORT
void free_aligned(void *mem);

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
  typedef std::ptrdiff_t difference_type;
  typedef std::size_t size_type;

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
    return (std::numeric_limits<size_type>::max)() / sizeof(T);
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
class VTKM_ALWAYS_EXPORT Storage<ValueT, vtkm::cont::StorageTagBasic>
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

  VTKM_CONT
  Storage(const ValueType *array = nullptr, vtkm::Id numberOfValues = 0);

  VTKM_CONT
  ~Storage();

  VTKM_CONT
  Storage(const Storage<ValueType, StorageTagBasic> &src);

  VTKM_CONT
  Storage &operator=(const Storage<ValueType, StorageTagBasic> &src);

  VTKM_CONT
  void ReleaseResources();

  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues);

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->NumberOfValues;
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues);

  VTKM_CONT
  PortalType GetPortal()
  {
    return PortalType(this->Array, this->Array + this->NumberOfValues);
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->Array, this->Array + this->NumberOfValues);
  }

  /// \brief Get a pointer to the underlying data structure.
  ///
  /// This method returns the pointer to the array held by this array. The
  /// memory associated with this array still belongs to the Storage (i.e.
  /// Storage will eventually deallocate the array).
  ///
  VTKM_CONT
  ValueType *GetArray()
  {
    return this->Array;
  }
  VTKM_CONT
  const ValueType *GetArray() const
  {
    return this->Array;
  }

  /// \brief Take the reference away from this object.
  ///
  /// This method returns the pointer to the array held by this array. It then
  /// clears the internal array pointer to nullptr, thereby ensuring that the
  /// Storage will never deallocate the array. This is helpful for taking a
  /// reference for an array created internally by VTK-m and not having to keep
  /// a VTK-m object around. Obviously the caller becomes responsible for
  /// destroying the memory.
  ///
  VTKM_CONT
  ValueType *StealArray();

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

#ifndef vtkm_cont_StorageBasic_cxx
namespace vtkm {
namespace cont {
namespace internal {

extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<char, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Int8, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::UInt8, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Int16, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::UInt16, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Int32, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::UInt32, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Int64, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::UInt64, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Float32, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Float64, StorageTagBasic>;

extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<vtkm::Int64,2>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<vtkm::Int32,2>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<vtkm::Float32,2>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<vtkm::Float64,2>, StorageTagBasic>;

extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<vtkm::Int64,3>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<vtkm::Int32,3>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<vtkm::Float32,3>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<vtkm::Float64,3>, StorageTagBasic>;

extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<char,4>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<Int8,4>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<UInt8,4>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<vtkm::Float32,4>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage< vtkm::Vec<vtkm::Float64,4>, StorageTagBasic>;
}
}
}
#endif

#include <vtkm/cont/StorageBasic.hxx>

#endif //vtk_m_cont_StorageBasic_h
