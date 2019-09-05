//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_PolicyBase_h
#define vtk_m_filter_PolicyBase_h

#include <vtkm/TypeListTag.h>

#include <vtkm/cont/CellSetListTag.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterListTag.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/StorageListTag.h>

#include <vtkm/filter/FilterTraits.h>

namespace vtkm
{
namespace filter
{

template <typename Derived>
struct PolicyBase
{
  using FieldTypeList = VTKM_DEFAULT_TYPE_LIST_TAG;
  using StorageList = vtkm::ListTagJoin<
    VTKM_DEFAULT_STORAGE_LIST_TAG,
    vtkm::ListTagBase<
      vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
      vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::Float32>,
                                              vtkm::cont::ArrayHandle<vtkm::Float32>,
                                              vtkm::cont::ArrayHandle<vtkm::Float32>>::StorageTag,
      vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::Float64>,
                                              vtkm::cont::ArrayHandle<vtkm::Float64>,
                                              vtkm::cont::ArrayHandle<vtkm::Float64>>::StorageTag>>;

  using StructuredCellSetList = vtkm::cont::CellSetListTagStructured;
  using UnstructuredCellSetList = vtkm::cont::CellSetListTagUnstructured;
  using AllCellSetList = VTKM_DEFAULT_CELL_SET_LIST_TAG;
};

namespace internal
{

namespace detail
{

// Given a base type, forms a list of all types with the same Vec structure but with the
// base component replaced with each of the basic C types.
template <typename BaseType>
struct AllCastingTypes
{
  using VTraits = vtkm::VecTraits<BaseType>;

  using type =
    vtkm::ListTagBase<typename VTraits::template ReplaceBaseComponentType<vtkm::Int8>,
                      typename VTraits::template ReplaceBaseComponentType<vtkm::UInt8>,
                      typename VTraits::template ReplaceBaseComponentType<vtkm::Int16>,
                      typename VTraits::template ReplaceBaseComponentType<vtkm::UInt8>,
                      typename VTraits::template ReplaceBaseComponentType<vtkm::Int32>,
                      typename VTraits::template ReplaceBaseComponentType<vtkm::UInt32>,
                      typename VTraits::template ReplaceBaseComponentType<vtkm::Int64>,
                      typename VTraits::template ReplaceBaseComponentType<vtkm::UInt64>,
                      typename VTraits::template ReplaceBaseComponentType<vtkm::Float32>,
                      typename VTraits::template ReplaceBaseComponentType<vtkm::Float64>>;
};

template <typename TargetT, typename SourceT, typename Storage, bool Valid>
struct CastArrayIfValid;

template <typename TargetT, typename SourceT, typename Storage>
struct CastArrayIfValid<TargetT, SourceT, Storage, true>
{
  using type = vtkm::cont::ArrayHandleCast<TargetT, vtkm::cont::ArrayHandle<SourceT, Storage>>;
};

template <typename TargetT, typename SourceT, typename Storage>
struct CastArrayIfValid<TargetT, SourceT, Storage, false>
{
  using type = vtkm::cont::ArrayHandleDiscard<TargetT>;
};

// Provides a transform template that builds a cast from an array of some source type to a
// cast array to a specific target type.
template <typename TargetT, typename Storage>
struct CastArrayTransform
{
  template <typename SourceT>
  using Transform = typename CastArrayIfValid<
    TargetT,
    SourceT,
    Storage,
    vtkm::cont::internal::IsValidArrayHandle<SourceT, Storage>::value>::type;
};

template <typename TargetT, typename Storage, bool Valid>
struct AllCastArraysForStorageImpl;

template <typename TargetT, typename Storage>
struct AllCastArraysForStorageImpl<TargetT, Storage, true>
{
  using SourceTypes = typename AllCastingTypes<TargetT>::type;
  using type = vtkm::ListTagJoin<
    vtkm::ListTagBase<vtkm::cont::ArrayHandle<TargetT, Storage>>,
    vtkm::ListTagTransform<SourceTypes, CastArrayTransform<TargetT, Storage>::template Transform>>;
};

template <typename TargetT, typename Storage>
struct AllCastArraysForStorageImpl<TargetT, Storage, false>
{
  using SourceTypes = typename AllCastingTypes<TargetT>::type;
  using type =
    vtkm::ListTagTransform<SourceTypes, CastArrayTransform<TargetT, Storage>::template Transform>;
};

// Special cases for known storage with limited type support.
template <>
struct AllCastArraysForStorageImpl<vtkm::Vec3f,
                                   vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
                                   true>
{
  using type = vtkm::ListTagBase<vtkm::cont::ArrayHandleUniformPointCoordinates>;
};
template <typename T>
struct AllCastArraysForStorageImpl<vtkm::Vec<T, 3>,
                                   vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
                                   false>
{
  using type = vtkm::ListTagBase<vtkm::cont::ArrayHandleCast<
    vtkm::Vec<T, 3>,
    vtkm::cont::ArrayHandle<vtkm::Vec3f,
                            vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag>>>;
};
template <typename TargetT>
struct AllCastArraysForStorageImpl<TargetT,
                                   vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
                                   false>
{
  using type = vtkm::ListTagEmpty;
};

template <typename T, typename S1, typename S2, typename S3>
struct AllCastArraysForStorageImpl<
  vtkm::Vec<T, 3>,
  vtkm::cont::internal::StorageTagCartesianProduct<vtkm::cont::ArrayHandle<T, S1>,
                                                   vtkm::cont::ArrayHandle<T, S2>,
                                                   vtkm::cont::ArrayHandle<T, S3>>,
  true>
{
  using type =
    vtkm::ListTagBase<vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<T, S1>,
                                                              vtkm::cont::ArrayHandle<T, S2>,
                                                              vtkm::cont::ArrayHandle<T, S3>>>;
};
template <typename TargetT, typename SourceT, typename S1, typename S2, typename S3>
struct AllCastArraysForStorageImpl<
  vtkm::Vec<TargetT, 3>,
  vtkm::cont::internal::StorageTagCartesianProduct<vtkm::cont::ArrayHandle<SourceT, S1>,
                                                   vtkm::cont::ArrayHandle<SourceT, S2>,
                                                   vtkm::cont::ArrayHandle<SourceT, S3>>,
  false>
{
  using type = vtkm::ListTagBase<vtkm::cont::ArrayHandleCast<
    vtkm::Vec<TargetT, 3>,
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<SourceT, S1>,
                                            vtkm::cont::ArrayHandle<SourceT, S2>,
                                            vtkm::cont::ArrayHandle<SourceT, S3>>>>;
};
template <typename TargetT, typename SourceT, typename S1, typename S2, typename S3>
struct AllCastArraysForStorageImpl<
  TargetT,
  vtkm::cont::internal::StorageTagCartesianProduct<vtkm::cont::ArrayHandle<SourceT, S1>,
                                                   vtkm::cont::ArrayHandle<SourceT, S2>,
                                                   vtkm::cont::ArrayHandle<SourceT, S3>>,
  false>
{
  using type = vtkm::ListTagEmpty;
};

// Given a target type and storage of an array handle, provides a list this array handle plus all
// array handles that can be cast to the target type wrapped in an ArrayHandleCast that does so.
template <typename TargetT, typename Storage>
struct AllCastArraysForStorage
{
  using SourceTypes = typename AllCastingTypes<TargetT>::type;
  using type = typename AllCastArraysForStorageImpl<
    TargetT,
    Storage,
    vtkm::cont::internal::IsValidArrayHandle<TargetT, Storage>::value>::type;
};

// Provides a transform template that converts a storage type to a list of all arrays that come
// from that storage type and can be cast to a target type (wrapped in an ArrayHandleCast as
// appropriate).
template <typename TargetT>
struct AllCastArraysTransform
{
  template <typename Storage>
  using Transform = typename AllCastArraysForStorage<TargetT, Storage>::type;
};

// Given a target type and a list of storage types, provides a joined list of all possible arrays
// of any of these storage cast to the target type.
template <typename TargetT, typename StorageList>
struct AllCastArraysForStorageList
{
  VTKM_IS_LIST_TAG(StorageList);
  using listOfLists =
    vtkm::ListTagTransform<StorageList, AllCastArraysTransform<TargetT>::template Transform>;
  using type = vtkm::ListTagApply<listOfLists, vtkm::ListTagJoin>;
};

} // detail

template <typename TargetT, typename StorageList>
using ArrayHandleMultiplexerForStorageList = vtkm::cont::ArrayHandleMultiplexerFromListTag<
  typename detail::AllCastArraysForStorageList<TargetT, StorageList>::type>;

} // namespace internal

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::VariantArrayHandleBase<typename DerivedPolicy::FieldTypeList> ApplyPolicy(
  const vtkm::cont::Field& field,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  using TypeList = typename DerivedPolicy::FieldTypeList;
  return field.GetData().ResetTypes(TypeList());
}

//-----------------------------------------------------------------------------
template <typename T, typename DerivedPolicy>
VTKM_CONT internal::ArrayHandleMultiplexerForStorageList<T, typename DerivedPolicy::StorageList>
ApplyPolicy(const vtkm::cont::Field& field, const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  using ArrayHandleMultiplexerType =
    internal::ArrayHandleMultiplexerForStorageList<T, typename DerivedPolicy::StorageList>;
  return field.GetData().AsMultiplexer<ArrayHandleMultiplexerType>();
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy, typename FilterType>
VTKM_CONT vtkm::cont::VariantArrayHandleBase<typename vtkm::filter::DeduceFilterFieldTypes<
  DerivedPolicy,
  typename vtkm::filter::FilterTraits<FilterType>::InputFieldTypeList>::TypeList>
ApplyPolicy(const vtkm::cont::Field& field,
            const vtkm::filter::PolicyBase<DerivedPolicy>&,
            const vtkm::filter::FilterTraits<FilterType>&)
{
  using FilterTypes = typename vtkm::filter::FilterTraits<FilterType>::InputFieldTypeList;
  using TypeList =
    typename vtkm::filter::DeduceFilterFieldTypes<DerivedPolicy, FilterTypes>::TypeList;
  return field.GetData().ResetTypes(TypeList());
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy, typename ListOfTypes>
VTKM_CONT vtkm::cont::VariantArrayHandleBase<
  typename vtkm::filter::DeduceFilterFieldTypes<DerivedPolicy, ListOfTypes>::TypeList>
ApplyPolicy(const vtkm::cont::Field& field,
            const vtkm::filter::PolicyBase<DerivedPolicy>&,
            const ListOfTypes&)
{
  using TypeList =
    typename vtkm::filter::DeduceFilterFieldTypes<DerivedPolicy, ListOfTypes>::TypeList;
  return field.GetData().ResetTypes(TypeList());
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::DynamicCellSetBase<typename DerivedPolicy::AllCellSetList> ApplyPolicy(
  const vtkm::cont::DynamicCellSet& cellset,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  using CellSetList = typename DerivedPolicy::AllCellSetList;
  return cellset.ResetCellSetList(CellSetList());
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::DynamicCellSetBase<typename DerivedPolicy::StructuredCellSetList>
ApplyPolicyStructured(const vtkm::cont::DynamicCellSet& cellset,
                      const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  using CellSetList = typename DerivedPolicy::StructuredCellSetList;
  return cellset.ResetCellSetList(CellSetList());
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::DynamicCellSetBase<typename DerivedPolicy::UnstructuredCellSetList>
ApplyPolicyUnstructured(const vtkm::cont::DynamicCellSet& cellset,
                        const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  using CellSetList = typename DerivedPolicy::UnstructuredCellSetList;
  return cellset.ResetCellSetList(CellSetList());
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::SerializableField<typename DerivedPolicy::FieldTypeList>
MakeSerializableField(const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return {};
}

template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::SerializableField<typename DerivedPolicy::FieldTypeList>
MakeSerializableField(const vtkm::cont::Field& field,
                      const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return vtkm::cont::SerializableField<typename DerivedPolicy::FieldTypeList>{ field };
}

template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::SerializableDataSet<typename DerivedPolicy::FieldTypeList,
                                          typename DerivedPolicy::AllCellSetList>
MakeSerializableDataSet(const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return {};
}

template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::SerializableDataSet<typename DerivedPolicy::FieldTypeList,
                                          typename DerivedPolicy::AllCellSetList>
MakeSerializableDataSet(const vtkm::cont::DataSet& dataset,
                        const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return vtkm::cont::SerializableDataSet<typename DerivedPolicy::FieldTypeList,
                                         typename DerivedPolicy::AllCellSetList>{ dataset };
}
}
} // vtkm::filter

#endif //vtk_m_filter_PolicyBase_h
