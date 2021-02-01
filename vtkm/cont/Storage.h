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

#include <vtkm/Flags.h>
#include <vtkm/StaticAssert.h>

#include <vtkm/internal/ArrayPortalDummy.h>

#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/Token.h>

#include <vtkm/cont/internal/Buffer.h>

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
/// for your new tag. Note that because the StorageTag is being used for
/// template specialization, storage tags cannot use inheritance (or, rather,
/// inheritance won't have any effect). You can, however, have a partial template
/// specialization of vtkm::cont::internal::Storage inherit from a different
/// specialization. So, for example, you could not have StorageTagFoo inherit from
/// StorageTagBase, but you could have vtkm::cont::internal::Storage<T, StorageTagFoo>
/// inherit from vtkm::cont::internal::Storage<T, StorageTagBase>.
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
  using ReadPortalType = vtkm::cont::internal::detail::UndefinedArrayPortal<T>;
  using WritePortalType = vtkm::cont::internal::detail::UndefinedArrayPortal<T>;
};
#else  //VTKM_DOXYGEN_ONLY
{
public:
  /// The type of each item in the array.
  ///
  using ValueType = T;

  /// \brief The type of portal objects for the array (read only).
  ///
  using ReadPortalType = vtkm::internal::ArrayPortalBasicRead<T>;

  /// \brief The type of portal objects for the array (read/write).
  ///
  using WritePortalType = vtkm::internal::ArrayPortalBasicWrite<T>;

  /// \brief Returns the number of buffers required for this storage.
  ///
  VTKM_CONT constexpr static vtkm::IdComponent GetNumberOfBuffers();

  /// \brief Resizes the array by changing the size of the buffers.
  ///
  /// Can also modify any metadata attached to the buffers.
  ///
  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      vtkm::cont::internal::Buffer* buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token);

  /// \brief Returns the number of entries allocated in the array.
  VTKM_CONT static vtkm::Id GetNumberOfValues(const vtkm::cont::internal::Buffer* buffers);

  /// \brief Create a read-only portal on the specified device.
  ///
  VTKM_CONT static ReadPortalType CreateReadPortal(const vtkm::cont::internal::Buffer* buffers,
                                                   vtkm::cont::DeviceAdapterId device,
                                                   vtkm::cont::Token& token);

  /// \brief Create a read/write portal on the specified device.
  ///
  VTKM_CONT static WritePortalType CreateWritePortal(vtkm::cont::internal::Buffer* buffers,
                                                     vtkm::cont::DeviceAdapterId device,
                                                     vtkm::cont::Token& token)
};
#endif // VTKM_DOXYGEN_ONLY

namespace detail
{

VTKM_CONT_EXPORT void StorageNoResizeImpl(vtkm::Id currentNumValues,
                                          vtkm::Id requestedNumValues,
                                          std::string storageTagName);

} // namespace detail

template <typename StorageType>
struct StorageTraits;

template <typename T, typename S>
struct StorageTraits<vtkm::cont::internal::Storage<T, S>>
{
  using ValueType = T;
  using Tag = S;
};

#define VTKM_STORAGE_NO_RESIZE                                                                     \
  VTKM_CONT static void ResizeBuffers(                                                             \
    vtkm::Id numValues, vtkm::cont::internal::Buffer* buffers, vtkm::CopyFlag, vtkm::cont::Token&) \
  {                                                                                                \
    vtkm::cont::internal::detail::StorageNoResizeImpl(                                             \
      GetNumberOfValues(buffers),                                                                  \
      numValues,                                                                                   \
      vtkm::cont::TypeToString<typename vtkm::cont::internal::StorageTraits<Storage>::Tag>());     \
  }                                                                                                \
  using ResizeBuffersEatComma = void

#define VTKM_STORAGE_NO_WRITE_PORTAL                                                           \
  using WritePortalType = vtkm::internal::ArrayPortalDummy<                                    \
    typename vtkm::cont::internal::StorageTraits<Storage>::ValueType>;                         \
  VTKM_CONT static WritePortalType CreateWritePortal(                                          \
    vtkm::cont::internal::Buffer*, vtkm::cont::DeviceAdapterId, vtkm::cont::Token&)            \
  {                                                                                            \
    throw vtkm::cont::ErrorBadAllocation(                                                      \
      "Cannot write to arrays with storage type of " +                                         \
      vtkm::cont::TypeToString<typename vtkm::cont::internal::StorageTraits<Storage>::Tag>()); \
  }                                                                                            \
  using CreateWritePortalEatComma = void

} // namespace internal
}
} // namespace vtkm::cont

#endif //vtk_m_cont_Storage_h
