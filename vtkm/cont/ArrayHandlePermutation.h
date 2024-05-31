//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandlePermutation_h
#define vtk_m_cont_ArrayHandlePermutation_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorBadValue.h>

namespace vtkm
{
namespace internal
{

template <typename IndexPortalType, typename ValuePortalType>
class VTKM_ALWAYS_EXPORT ArrayPortalPermutation
{
  using Writable = vtkm::internal::PortalSupportsSets<ValuePortalType>;

public:
  using ValueType = typename ValuePortalType::ValueType;

  VTKM_EXEC_CONT
  ArrayPortalPermutation()
    : IndexPortal()
    , ValuePortal()
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalPermutation(const IndexPortalType& indexPortal, const ValuePortalType& valuePortal)
    : IndexPortal(indexPortal)
    , ValuePortal(valuePortal)
  {
  }

  /// Copy constructor for any other ArrayPortalPermutation with delegate
  /// portal types that can be copied to these portal types. This allows us to
  /// do any type casting that the delegate portals do (like the non-const to
  /// const cast).
  ///
  template <typename OtherIP, typename OtherVP>
  VTKM_EXEC_CONT ArrayPortalPermutation(const ArrayPortalPermutation<OtherIP, OtherVP>& src)
    : IndexPortal(src.GetIndexPortal())
    , ValuePortal(src.GetValuePortal())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->IndexPortal.GetNumberOfValues(); }

  VTKM_EXEC
  ValueType Get(vtkm::Id index) const
  {
    vtkm::Id permutedIndex = this->IndexPortal.Get(index);
    return this->ValuePortal.Get(permutedIndex);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC void Set(vtkm::Id index, const ValueType& value) const
  {
    vtkm::Id permutedIndex = this->IndexPortal.Get(index);
    this->ValuePortal.Set(permutedIndex, value);
  }

  VTKM_EXEC_CONT
  const IndexPortalType& GetIndexPortal() const { return this->IndexPortal; }

  VTKM_EXEC_CONT
  const ValuePortalType& GetValuePortal() const { return this->ValuePortal; }

private:
  IndexPortalType IndexPortal;
  ValuePortalType ValuePortal;
};
}
} // namespace vtkm::internal

namespace vtkm
{
namespace cont
{

template <typename IndexStorageTag, typename ValueStorageTag>
struct VTKM_ALWAYS_EXPORT StorageTagPermutation
{
};

namespace internal
{

template <typename T, typename IndexStorageTag, typename ValueStorageTag>
class Storage<T, vtkm::cont::StorageTagPermutation<IndexStorageTag, ValueStorageTag>>
{
  VTKM_STATIC_ASSERT_MSG(
    (vtkm::cont::internal::IsValidArrayHandle<vtkm::Id, IndexStorageTag>::value),
    "Invalid index storage tag.");
  VTKM_STATIC_ASSERT_MSG((vtkm::cont::internal::IsValidArrayHandle<T, ValueStorageTag>::value),
                         "Invalid value storage tag.");

  using IndexStorage = vtkm::cont::internal::Storage<vtkm::Id, IndexStorageTag>;
  using ValueStorage = vtkm::cont::internal::Storage<T, ValueStorageTag>;

  using IndexArray = vtkm::cont::ArrayHandle<vtkm::Id, IndexStorageTag>;
  using ValueArray = vtkm::cont::ArrayHandle<T, ValueStorageTag>;

  struct Info
  {
    std::size_t ValueBufferOffset;
  };

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> IndexBuffers(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<vtkm::cont::internal::Buffer>(buffers.begin() + 1,
                                                     buffers.begin() + info.ValueBufferOffset);
  }
  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> ValueBuffers(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<vtkm::cont::internal::Buffer>(buffers.begin() + info.ValueBufferOffset,
                                                     buffers.end());
  }

public:
  VTKM_STORAGE_NO_RESIZE;

  using ReadPortalType =
    vtkm::internal::ArrayPortalPermutation<typename IndexStorage::ReadPortalType,
                                           typename ValueStorage::ReadPortalType>;
  using WritePortalType =
    vtkm::internal::ArrayPortalPermutation<typename IndexStorage::ReadPortalType,
                                           typename ValueStorage::WritePortalType>;

  VTKM_CONT static vtkm::IdComponent GetNumberOfComponentsFlat(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return ValueStorage::GetNumberOfComponentsFlat(ValueBuffers(buffers));
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return IndexStorage::GetNumberOfValues(IndexBuffers(buffers));
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>&,
                             const T&,
                             vtkm::Id,
                             vtkm::Id,
                             vtkm::cont::Token&)
  {
    throw vtkm::cont::ErrorBadType("Fill not supported for ArrayHandlePermutation.");
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return ReadPortalType(IndexStorage::CreateReadPortal(IndexBuffers(buffers), device, token),
                          ValueStorage::CreateReadPortal(ValueBuffers(buffers), device, token));
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    // Note: the index portal is always a read-only portal.
    return WritePortalType(IndexStorage::CreateReadPortal(IndexBuffers(buffers), device, token),
                           ValueStorage::CreateWritePortal(ValueBuffers(buffers), device, token));
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(
    const IndexArray& indexArray = IndexArray{},
    const ValueArray& valueArray = ValueArray{})
  {
    Info info;
    info.ValueBufferOffset = 1 + indexArray.GetBuffers().size();
    return vtkm::cont::internal::CreateBuffers(info, indexArray, valueArray);
  }

  VTKM_CONT static IndexArray GetIndexArray(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return IndexArray(IndexBuffers(buffers));
  }

  VTKM_CONT static ValueArray GetValueArray(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return ValueArray(ValueBuffers(buffers));
  }
};

} // namespace internal

/// \brief Implicitly permutes the values in an array.
///
/// ArrayHandlePermutation is a specialization of ArrayHandle. It takes two
/// delegate array handles: an array of indices and an array of values. The
/// array handle created contains the values given permuted by the indices
/// given. So for a given index i, ArrayHandlePermutation looks up the i-th
/// value in the index array to get permuted index j and then gets the j-th
/// value in the value array. This index permutation is done on the fly rather
/// than creating a copy of the array.
///
/// An ArrayHandlePermutation can be used for either input or output. However,
/// if used for output the array must be pre-allocated. That is, the indices
/// must already be established and the values must have an allocation large
/// enough to accommodate the indices. An output ArrayHandlePermutation will
/// only have values changed. The indices are never changed.
///
/// When using ArrayHandlePermutation great care should be taken to make sure
/// that every index in the index array points to a valid position in the value
/// array. Otherwise, access validations will occur. Also, be wary of duplicate
/// indices that point to the same location in the value array. For input
/// arrays, this is fine. However, this could result in unexpected results for
/// using as output and is almost certainly wrong for using as in-place.
///
template <typename IndexArrayHandleType, typename ValueArrayHandleType>
class ArrayHandlePermutation
  : public vtkm::cont::ArrayHandle<
      typename ValueArrayHandleType::ValueType,
      vtkm::cont::StorageTagPermutation<typename IndexArrayHandleType::StorageTag,
                                        typename ValueArrayHandleType::StorageTag>>
{
  // If the following line gives a compile error, then the ArrayHandleType
  // template argument is not a valid ArrayHandle type.
  VTKM_IS_ARRAY_HANDLE(IndexArrayHandleType);
  VTKM_IS_ARRAY_HANDLE(ValueArrayHandleType);

  VTKM_STATIC_ASSERT_MSG(
    (std::is_same<vtkm::Id, typename IndexArrayHandleType::ValueType>::value),
    "Permutation array in ArrayHandlePermutation must have vtkm::Id value type.");

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandlePermutation,
    (ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>),
    (vtkm::cont::ArrayHandle<
      typename ValueArrayHandleType::ValueType,
      vtkm::cont::StorageTagPermutation<typename IndexArrayHandleType::StorageTag,
                                        typename ValueArrayHandleType::StorageTag>>));

  /// Construct a permuation array with index and value arrays.
  VTKM_CONT
  ArrayHandlePermutation(const IndexArrayHandleType& indexArray,
                         const ValueArrayHandleType& valueArray)
    : Superclass(StorageType::CreateBuffers(indexArray, valueArray))
  {
  }

  /// @brief Return the array used for indices.
  ///
  /// The index array provides how indices get permuted. When a value is retrieved from an
  /// `ArrayHandlePermutation`, an index is retrived from this index array, and this new
  /// index is used to retrieve a value from the value array.
  VTKM_CONT IndexArrayHandleType GetIndexArray() const
  {
    return StorageType::GetIndexArray(this->GetBuffers());
  }

  /// @brief Return the array used for values.
  ///
  /// The index array provides how indices get permuted. When a value is retrieved from an
  /// `ArrayHandlePermutation`, an index is retrived from this index array, and this new
  /// index is used to retrieve a value from the value array.
  VTKM_CONT ValueArrayHandleType GetValueArray() const
  {
    return StorageType::GetValueArray(this->GetBuffers());
  }
};

/// make_ArrayHandleTransform is convenience function to generate an
/// ArrayHandleTransform.  It takes in an ArrayHandle and a functor
/// to apply to each element of the Handle.
template <typename IndexArrayHandleType, typename ValueArrayHandleType>
VTKM_CONT vtkm::cont::ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>
make_ArrayHandlePermutation(IndexArrayHandleType indexArray, ValueArrayHandleType valueArray)
{
  return ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>(indexArray, valueArray);
}

}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename IdxAH, typename ValAH>
struct SerializableTypeString<vtkm::cont::ArrayHandlePermutation<IdxAH, ValAH>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Permutation<" + SerializableTypeString<IdxAH>::Get() + "," +
      SerializableTypeString<ValAH>::Get() + ">";
    return name;
  }
};

template <typename T, typename IdxST, typename ValST>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagPermutation<IdxST, ValST>>>
  : SerializableTypeString<
      vtkm::cont::ArrayHandlePermutation<vtkm::cont::ArrayHandle<vtkm::Id, IdxST>,
                                         vtkm::cont::ArrayHandle<T, ValST>>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename IdxAH, typename ValAH>
struct Serialization<vtkm::cont::ArrayHandlePermutation<IdxAH, ValAH>>
{
private:
  using Type = vtkm::cont::ArrayHandlePermutation<IdxAH, ValAH>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, Type(obj).GetIndexArray());
    vtkmdiy::save(bb, Type(obj).GetValueArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    IdxAH indices;
    ValAH values;

    vtkmdiy::load(bb, indices);
    vtkmdiy::load(bb, values);

    obj = vtkm::cont::make_ArrayHandlePermutation(indices, values);
  }
};

template <typename T, typename IdxST, typename ValST>
struct Serialization<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagPermutation<IdxST, ValST>>>
  : Serialization<vtkm::cont::ArrayHandlePermutation<vtkm::cont::ArrayHandle<vtkm::Id, IdxST>,
                                                     vtkm::cont::ArrayHandle<T, ValST>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandlePermutation_h
