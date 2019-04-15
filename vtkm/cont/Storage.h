//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_Storage_h
#define vtk_m_cont_Storage_h

#define VTKM_STORAGE_ERROR -2
#define VTKM_STORAGE_UNDEFINED -1
#define VTKM_STORAGE_BASIC 1

#ifndef VTKM_STORAGE
#define VTKM_STORAGE VTKM_STORAGE_BASIC
#endif

#include <vtkm/StaticAssert.h>

#include <vtkm/cont/vtkm_cont_export.h>
#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{
namespace cont
{

#ifdef VTKM_DOXYGEN_ONLY
/// \brief A tag specifying client memory allocation.
///
/// A Storage tag specifies how an ArrayHandle allocates and frees memory. The
/// tag StorageTag___ does not actually exist. Rather, this documentation is
/// provided to describe how array storage objects are specified. Loading the
/// vtkm/cont/Storage.h header will set a default array storage. You can
/// specify the default storage by first setting the VTKM_STORAGE macro.
/// Currently it can only be set to VTKM_STORAGE_BASIC.
///
/// User code external to VTK-m is free to make its own StorageTag. This is a
/// good way to get VTK-m to read data directly in and out of arrays from other
/// libraries. However, care should be taken when creating a Storage. One
/// particular problem that is likely is a storage that "constructs" all the
/// items in the array. If done incorrectly, then memory of the array can be
/// incorrectly bound to the wrong processor. If you do provide your own
/// StorageTag, please be diligent in comparing its performance to the
/// StorageTagBasic.
///
/// To implement your own StorageTag, you first must create a tag class (an
/// empty struct) defining your tag (i.e. struct VTKM_ALWAYS_EXPORT StorageTagMyAlloc { };). Then
/// provide a partial template specialization of vtkm::cont::internal::Storage
/// for your new tag.
///
struct VTKM_ALWAYS_EXPORT StorageTag___
{
};
#endif // VTKM_DOXYGEN_ONLY

namespace internal
{

struct UndefinedStorage
{
};

namespace detail
{

// This class should never be used. It is used as a placeholder for undefined
// Storage objects. If you get a compiler error involving this object, then it
// probably comes from trying to use an ArrayHandle with bad template
// arguments.
template <typename T>
struct UndefinedArrayPortal
{
  VTKM_STATIC_ASSERT(sizeof(T) == static_cast<size_t>(-1));
};

} // namespace detail

/// This templated class must be partially specialized for each StorageTag
/// created, which will define the implementation for that tag.
///
template <typename T, class StorageTag>
class Storage
#ifndef VTKM_DOXYGEN_ONLY
  : public vtkm::cont::internal::UndefinedStorage
{
public:
  using PortalType = vtkm::cont::internal::detail::UndefinedArrayPortal<T>;
  using PortalConstType = vtkm::cont::internal::detail::UndefinedArrayPortal<T>;
};
#else  //VTKM_DOXYGEN_ONLY
{
public:
  /// The type of each item in the array.
  ///
  using ValueType = T;

  /// \brief The type of portal objects for the array.
  ///
  /// The actual portal object can take any form. This is a simple example of a
  /// portal to a C array.
  ///
  using PortalType = ::vtkm::cont::internal::ArrayPortalFromIterators<ValueType*>;

  /// \brief The type of portal objects (const version) for the array.
  ///
  /// The actual portal object can take any form. This is a simple example of a
  /// portal to a C array.
  ///
  using PortalConstType = ::vtkm::cont::internal::ArrayPortalFromIterators<const ValueType*>;

  VTKM_CONT Storage(const Storage& src);
  VTKM_CONT Storage(Storage&& src) noexcept;
  VTKM_CONT Storage& operator=(const Storage& src);
  VTKM_CONT Storage& operator=(Storage&& src);

  /// Returns a portal to the array.
  ///
  VTKM_CONT
  PortalType GetPortal();

  /// Returns a portal to the array with immutable values.
  ///
  VTKM_CONT
  PortalConstType GetPortalConst() const;

  /// Returns the number of entries allocated in the array.
  VTKM_CONT
  vtkm::Id GetNumberOfValues() const;

  /// \brief Allocates an array large enough to hold the given number of values.
  ///
  /// The allocation may be done on an already existing array, but can wipe out
  /// any data already in the array. This method can throw
  /// ErrorBadAllocation if the array cannot be allocated or
  /// ErrorBadValue if the allocation is not feasible (for example, the
  /// array storage is read-only).
  ///
  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues);

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues);

  /// \brief Frees any resources (i.e. memory) stored in this array.
  ///
  /// After calling this method GetNumberOfValues will return 0. The
  /// resources should also be released when the Storage class is
  /// destroyed.
  VTKM_CONT
  void ReleaseResources();
};
#endif // VTKM_DOXYGEN_ONLY

} // namespace internal
}
} // namespace vtkm::cont

// This is put at the bottom of the header so that the Storage template is
// declared before any implementations are called.

#if VTKM_STORAGE == VTKM_STORAGE_BASIC

#include <vtkm/cont/StorageBasic.h>
#define VTKM_DEFAULT_STORAGE_TAG ::vtkm::cont::StorageTagBasic

#elif VTKM_STORAGE == VTKM_STORAGE_ERROR

#include <vtkm/cont/internal/StorageError.h>
#define VTKM_DEFAULT_STORAGE_TAG ::vtkm::cont::internal::StorageTagError

#elif (VTKM_STORAGE == VTKM_STORAGE_UNDEFINED) || !defined(VTKM_STORAGE)

#ifndef VTKM_DEFAULT_STORAGE_TAG
#warning If array storage is undefined, VTKM_DEFAULT_STORAGE_TAG must be defined.
#endif

#endif

#endif //vtk_m_cont_Storage_h
