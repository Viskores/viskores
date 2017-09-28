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
#ifndef vtk_m_cont_StorageBasic_h
#define vtk_m_cont_StorageBasic_h

#include <vtkm/Assert.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Storage.h>

#include <vtkm/cont/internal/ArrayPortalFromIterators.h>

// Defines the cache line size in bytes to align allocations to
#ifndef VTKM_CACHE_LINE_SIZE
#define VTKM_CACHE_LINE_SIZE 64
#endif

namespace vtkm
{
namespace cont
{

/// A tag for the basic implementation of a Storage object.
struct VTKM_ALWAYS_EXPORT StorageTagBasic
{
};

namespace internal
{

VTKM_CONT_EXPORT
void* alloc_aligned(size_t size, size_t align);

VTKM_CONT_EXPORT
void free_aligned(void* mem);

/// \brief an aligned allocator
/// A simple aligned allocator type that will align allocations to `Alignment` bytes
/// TODO: Once C++11 std::allocator_traits is better used by STL and we want to drop
/// support for pre-C++11 we can drop a lot of the typedefs and functions here.
template <typename T, size_t Alignment>
struct AlignedAllocator
{
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using void_pointer = void*;
  using const_void_pointer = const void*;
  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;

  template <typename U>
  struct rebind
  {
    using other = AlignedAllocator<U, Alignment>;
  };

  AlignedAllocator() {}

  template <typename Tb>
  AlignedAllocator(const AlignedAllocator<Tb, Alignment>&)
  {
  }

  pointer allocate(size_t n)
  {
    return static_cast<pointer>(alloc_aligned(n * sizeof(T), Alignment));
  }
  void deallocate(pointer p, size_t) { free_aligned(static_cast<void*>(p)); }
  pointer address(reference r) { return &r; }
  const_pointer address(const_reference r) { return &r; }
  size_type max_size() const { return (std::numeric_limits<size_type>::max)() / sizeof(T); }
  void construct(pointer p, const T& t)
  {
    (void)p;
    new (p) T(t);
  }
  void destroy(pointer p)
  {
    (void)p;
    p->~T();
  }
};

template <typename T, typename U, size_t AlignA, size_t AlignB>
bool operator==(const AlignedAllocator<T, AlignA>&, const AlignedAllocator<U, AlignB>&)
{
  return AlignA == AlignB;
}
template <typename T, typename U, size_t AlignA, size_t AlignB>
bool operator!=(const AlignedAllocator<T, AlignA>&, const AlignedAllocator<U, AlignB>&)
{
  return AlignA != AlignB;
}

/// Base class for basic storage classes. This is currently only used by
/// Basic storage to provide a type-agnostic API for allocations, etc.
class VTKM_CONT_EXPORT StorageBasicBase
{
public:
  StorageBasicBase() {}
  virtual ~StorageBasicBase();

  /// \brief Return the number of bytes allocated for this storage object.
  VTKM_CONT
  virtual vtkm::UInt64 GetNumberOfBytes() const = 0;

  /// \brief Allocates an array with the specified size in bytes.
  ///
  /// The allocation may be done on an already existing array, but can wipe out
  /// any data already in the array. This method can throw
  /// ErrorBadAllocation if the array cannot be allocated or
  /// ErrorBadValue if the allocation is not feasible (for example, the
  /// array storage is read-only).
  VTKM_CONT
  virtual void AllocateBytes(vtkm::UInt64 numberOfBytes) = 0;

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// size of the array is changed to \c numberOfBytes bytes. The data
  /// in the reallocated array stays the same, but \c numberOfBytes must be
  /// equal or less than the preexisting size. That is, this method can only be
  /// used to shorten the array, not lengthen.
  VTKM_CONT
  virtual void ShrinkBytes(vtkm::UInt64 numberOfBytes) = 0;

  /// \brief Frees any resources (i.e. memory) stored in this array.
  ///
  /// After calling this method GetNumberOfBytes() will return 0. The
  /// resources should also be released when the Storage class is
  /// destroyed.
  VTKM_CONT
  virtual void ReleaseResources() = 0;

  /// Return the memory location of the first element of the array data.
  VTKM_CONT
  virtual void* GetBasePointer() const = 0;

  /// Return the memory location of the first element past the end of the array
  /// data.
  VTKM_CONT
  virtual void* GetEndPointer() const = 0;

  /// Return the memory location of the first element past the end of the
  /// array's allocated memory buffer.
  VTKM_CONT
  virtual void* GetCapacityPointer() const = 0;
};

/// A basic implementation of an Storage object.
///
/// \todo This storage does \em not construct the values within the array.
/// Thus, it is important to not use this class with any type that will fail if
/// not constructed. These are things like basic types (int, float, etc.) and
/// the VTKm Tuple classes.  In the future it would be nice to have a compile
/// time check to enforce this.
///
template <typename ValueT>
class VTKM_ALWAYS_EXPORT Storage<ValueT, vtkm::cont::StorageTagBasic> : public StorageBasicBase
{
public:
  using ValueType = ValueT;
  using PortalType = vtkm::cont::internal::ArrayPortalFromIterators<ValueType*>;
  using PortalConstType = vtkm::cont::internal::ArrayPortalFromIterators<const ValueType*>;

  /// The original design of this class provided an allocator as a template
  /// parameters. That messed things up, though, because other templated
  /// classes assume that the \c Storage has one template parameter. There are
  /// other ways to allow you to specify the allocator, but it is uncertain
  /// whether that would ever be useful. So, instead of jumping through hoops
  /// implementing them, just fix the allocator for now.
  ///
  using AllocatorType = AlignedAllocator<ValueType, VTKM_CACHE_LINE_SIZE>;

public:
  /// \brief construct storage that VTK-m is responsible for
  VTKM_CONT
  Storage();

  /// \brief construct storage that VTK-m is not responsible for
  VTKM_CONT
  Storage(const ValueType* array, vtkm::Id numberOfValues = 0);

  VTKM_CONT
  ~Storage();

  VTKM_CONT
  Storage(const Storage<ValueType, StorageTagBasic>& src);

  VTKM_CONT
  Storage& operator=(const Storage<ValueType, StorageTagBasic>& src);

  VTKM_CONT
  void ReleaseResources() final;

  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues);

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_CONT
  vtkm::UInt64 GetNumberOfBytes() const final
  {
    return static_cast<vtkm::UInt64>(this->NumberOfValues) *
      static_cast<vtkm::UInt64>(sizeof(ValueT));
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues);

  VTKM_CONT
  void AllocateBytes(vtkm::UInt64) final;

  VTKM_CONT
  void ShrinkBytes(vtkm::UInt64) final;

  VTKM_CONT
  PortalType GetPortal() { return PortalType(this->Array, this->Array + this->NumberOfValues); }

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
  ValueType* GetArray() { return this->Array; }
  VTKM_CONT
  const ValueType* GetArray() const { return this->Array; }

  /// \brief Take the reference away from this object.
  ///
  /// This method returns the pointer to the array held by this array. It then
  /// clears the internal ownership flags, thereby ensuring that the
  /// Storage will never deallocate the array or be able to reallocate it. This
  /// is helpful for taking a reference for an array created internally by
  /// VTK-m and not having to keep a VTK-m object around. Obviously the caller
  /// becomes responsible for destroying the memory.
  ///
  VTKM_CONT
  ValueType* StealArray();

  /// \brief Returns if vtkm will deallocate this memory. VTK-m StorageBasic
  /// is designed that VTK-m will not deallocate user passed memory, or
  /// instances that have been stolen (\c StealArray)
  VTKM_CONT
  bool WillDeallocate() const { return this->DeallocateOnRelease; }


  VTKM_CONT
  void* GetBasePointer() const final { return static_cast<void*>(this->Array); }

  VTKM_CONT
  void* GetEndPointer() const final
  {
    return static_cast<void*>(this->Array + this->NumberOfValues);
  }

  VTKM_CONT
  void* GetCapacityPointer() const final
  {
    return static_cast<void*>(this->Array + this->AllocatedSize);
  }

private:
  ValueType* Array;
  vtkm::Id NumberOfValues;
  vtkm::Id AllocatedSize;
  bool DeallocateOnRelease;
};

} // namespace internal
}
} // namespace vtkm::cont

#ifndef vtkm_cont_StorageBasic_cxx
namespace vtkm
{
namespace cont
{
namespace internal
{

/// \cond
/// Make doxygen ignore this section
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

extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Vec<vtkm::Int64, 2>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Vec<vtkm::Int32, 2>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT
  Storage<vtkm::Vec<vtkm::Float32, 2>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT
  Storage<vtkm::Vec<vtkm::Float64, 2>, StorageTagBasic>;

extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Vec<vtkm::Int64, 3>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Vec<vtkm::Int32, 3>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT
  Storage<vtkm::Vec<vtkm::Float32, 3>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT
  Storage<vtkm::Vec<vtkm::Float64, 3>, StorageTagBasic>;

extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Vec<char, 4>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Vec<Int8, 4>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT Storage<vtkm::Vec<UInt8, 4>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT
  Storage<vtkm::Vec<vtkm::Float32, 4>, StorageTagBasic>;
extern template class VTKM_CONT_TEMPLATE_EXPORT
  Storage<vtkm::Vec<vtkm::Float64, 4>, StorageTagBasic>;
/// \endcond
}
}
}
#endif

#include <vtkm/cont/StorageBasic.hxx>

#endif //vtk_m_cont_StorageBasic_h
