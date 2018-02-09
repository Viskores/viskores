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

/// Class that does all of VTK-m allocations
/// for storage basic. This is exists so that
/// stolen arrays can call the correct free
/// function ( _aligned_malloc ) on windows
struct VTKM_CONT_EXPORT StorageBasicAllocator
{
  void* allocate(size_t size, size_t align);
  void free_memory(void* p);

  template <typename T>
  void deallocate(T* p)
  {
    this->free_memory(static_cast<void*>(p));
  }
};
/// Base class for basic storage classes. This allow us to implement
/// vtkm::cont::Storage<T, StorageTagBasic > for any T type with no overhead
/// as all heavy logic is provide by a type-agnostic API including allocations, etc.
class VTKM_CONT_EXPORT StorageBasicBase
{
public:
  using AllocatorType = StorageBasicAllocator;
  VTKM_CONT StorageBasicBase();
  VTKM_CONT StorageBasicBase(const void* array, vtkm::Id size, vtkm::UInt64 sizeOfValue);
  VTKM_CONT ~StorageBasicBase();

  VTKM_CONT StorageBasicBase(const StorageBasicBase& src);
  VTKM_CONT StorageBasicBase operator=(const StorageBasicBase& src);

  /// \brief Return the number of bytes allocated for this storage object(Capacity).
  ///
  ///
  VTKM_CONT vtkm::UInt64 GetNumberOfBytes() const { return this->AllocatedByteSize; }

  /// \brief Return the number of 'T' values allocated by this storage
  VTKM_CONT vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  /// \brief Allocates an array with the specified number of elements.
  ///
  /// The allocation may be done on an already existing array, but can wipe out
  /// any data already in the array. This method can throw
  /// ErrorBadAllocation if the array cannot be allocated or
  /// ErrorBadValue if the allocation is not feasible (for example, the
  /// array storage is read-only).
  VTKM_CONT void AllocateValues(vtkm::Id numberOfValues, vtkm::UInt64 sizeOfValue);

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// size of the array is changed so that it can hold \c numberOfValues values.
  /// The data in the reallocated array stays the same, but \c numberOfValues must be
  /// equal or less than the preexisting size. That is, this method can only be
  /// used to shorten the array, not lengthen.
  VTKM_CONT void Shrink(vtkm::Id numberOfValues);

  /// \brief Frees any resources (i.e. memory) stored in this array.
  ///
  /// After calling this method GetNumberOfBytes() will return 0. The
  /// resources should also be released when the Storage class is
  /// destroyed.
  VTKM_CONT void ReleaseResources();

  /// \brief Returns if vtkm will deallocate this memory. VTK-m StorageBasic
  /// is designed that VTK-m will not deallocate user passed memory, or
  /// instances that have been stolen (\c StealArray)
  VTKM_CONT bool WillDeallocate() const { return this->DeallocateOnRelease; }

  /// Return the memory location of the first element of the array data.
  VTKM_CONT void* GetBasePointer() const;

  VTKM_CONT void* GetEndPointer(vtkm::Id numberOfValues, vtkm::UInt64 sizeOfValue) const;

  /// Return the memory location of the first element past the end of the
  /// array's allocated memory buffer.
  VTKM_CONT void* GetCapacityPointer() const;

protected:
  void* Array;
  vtkm::UInt64 AllocatedByteSize;
  vtkm::Id NumberOfValues;
  bool DeallocateOnRelease;
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
  using AllocatorType = vtkm::cont::internal::StorageBasicAllocator;
  using ValueType = ValueT;
  using PortalType = vtkm::cont::internal::ArrayPortalFromIterators<ValueType*>;
  using PortalConstType = vtkm::cont::internal::ArrayPortalFromIterators<const ValueType*>;

public:
  /// \brief construct storage that VTK-m is responsible for
  VTKM_CONT Storage();

  /// \brief construct storage that VTK-m is not responsible for
  VTKM_CONT Storage(const ValueType* array, vtkm::Id numberOfValues = 0);

  VTKM_CONT void Allocate(vtkm::Id numberOfValues);

  VTKM_CONT PortalType GetPortal();

  VTKM_CONT PortalConstType GetPortalConst() const;

  /// \brief Get a pointer to the underlying data structure.
  ///
  /// This method returns the pointer to the array held by this array. The
  /// memory associated with this array still belongs to the Storage (i.e.
  /// Storage will eventually deallocate the array).
  ///
  VTKM_CONT ValueType* GetArray();

  VTKM_CONT const ValueType* GetArray() const;

  /// \brief Take the reference away from this object.
  ///
  /// This method returns the pointer to the array held by this array. It then
  /// clears the internal ownership flags, thereby ensuring that the
  /// Storage will never deallocate the array or be able to reallocate it. This
  /// is helpful for taking a reference for an array created internally by
  /// VTK-m and not having to keep a VTK-m object around. Obviously the caller
  /// becomes responsible for destroying the memory.
  ///
  VTKM_CONT ValueType* StealArray();
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
