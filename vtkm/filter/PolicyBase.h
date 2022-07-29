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

#include <vtkm/List.h>

#include <vtkm/cont/CellSetList.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DefaultTypes.h>
#include <vtkm/cont/DeviceAdapterList.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/StorageList.h>
#include <vtkm/cont/UncertainCellSet.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <vtkm/filter/FilterTraits.h>

namespace vtkm
{
namespace filter
{

template <typename Derived>
struct PolicyBase
{
  using FieldTypeList = vtkm::ListUniversal;
  using StorageList = VTKM_DEFAULT_STORAGE_LIST;

  using StructuredCellSetList = VTKM_DEFAULT_CELL_SET_LIST_STRUCTURED;
  using UnstructuredCellSetList = VTKM_DEFAULT_CELL_SET_LIST_UNSTRUCTURED;
  using AllCellSetList = VTKM_DEFAULT_CELL_SET_LIST;
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

  using type = vtkm::List<typename VTraits::template ReplaceBaseComponentType<vtkm::Int8>,
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

// Provides a transform template that builds a cast from an array of some source type to a
// cast array to a specific target type.
template <typename TargetT, typename Storage>
struct CastArrayTransform
{
  template <typename SourceT>
  using Transform = vtkm::cont::ArrayHandleCast<TargetT, vtkm::cont::ArrayHandle<SourceT, Storage>>;
};

// Provides a predicate for a particular storage that resolves to std::true_type if a given
// type cannot be used with the storage.
template <typename Storage>
struct ArrayValidPredicate
{
  template <typename T>
  using Predicate = vtkm::cont::internal::IsInvalidArrayHandle<T, Storage>;
};

template <typename TargetT, typename Storage, bool Valid>
struct AllCastArraysForStorageImpl;

template <typename TargetT, typename Storage>
struct ValidCastingTypes
{
  using type = vtkm::ListRemoveIf<typename AllCastingTypes<TargetT>::type,
                                  ArrayValidPredicate<Storage>::template Predicate>;
};

template <typename TargetT, typename Storage>
struct AllCastArraysForStorageImpl<TargetT, Storage, true>
{
  using SourceTypes = typename ValidCastingTypes<TargetT, Storage>::type;
  using CastArrays =
    vtkm::ListTransform<SourceTypes, CastArrayTransform<TargetT, Storage>::template Transform>;
  using type = vtkm::ListAppend<vtkm::List<vtkm::cont::ArrayHandle<TargetT, Storage>>, CastArrays>;
};

template <typename TargetT, typename Storage>
struct AllCastArraysForStorageImpl<TargetT, Storage, false>
{
  using SourceTypes = typename ValidCastingTypes<TargetT, Storage>::type;
  using type =
    vtkm::ListTransform<SourceTypes, CastArrayTransform<TargetT, Storage>::template Transform>;
};

// Special cases for known storage with limited type support.
template <>
struct AllCastArraysForStorageImpl<vtkm::Vec3f,
                                   vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
                                   true>
{
  using type = vtkm::List<vtkm::cont::ArrayHandleUniformPointCoordinates>;
};
template <typename T>
struct AllCastArraysForStorageImpl<vtkm::Vec<T, 3>,
                                   vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
                                   false>
{
  using type = vtkm::List<vtkm::cont::ArrayHandleCast<
    vtkm::Vec<T, 3>,
    vtkm::cont::ArrayHandle<vtkm::Vec3f,
                            vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag>>>;
};
template <typename TargetT>
struct AllCastArraysForStorageImpl<TargetT,
                                   vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
                                   false>
{
  using type = vtkm::ListEmpty;
};

template <typename T, typename S1, typename S2, typename S3>
struct AllCastArraysForStorageImpl<vtkm::Vec<T, 3>,
                                   vtkm::cont::StorageTagCartesianProduct<S1, S2, S3>,
                                   true>
{
  using type = vtkm::List<vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<T, S1>,
                                                                  vtkm::cont::ArrayHandle<T, S2>,
                                                                  vtkm::cont::ArrayHandle<T, S3>>>;
};
template <typename TargetT, typename S1, typename S2, typename S3>
struct AllCastArraysForStorageImpl<TargetT,
                                   vtkm::cont::StorageTagCartesianProduct<S1, S2, S3>,
                                   false>
{
  using type = vtkm::ListEmpty;
};

#ifdef VTKM_ADD_XGC_DEFAULT_TYPES
template <typename T>
struct AllCastArraysForStorageImpl<vtkm::Vec<T, 3>, vtkm::cont::StorageTagXGCCoordinates, true>
{
  using type = vtkm::List<vtkm::cont::ArrayHandleXGCCoordinates<T>>;
};
template <typename TargetT>
struct AllCastArraysForStorageImpl<TargetT, vtkm::cont::StorageTagXGCCoordinates, false>
{
  using type = vtkm::ListEmpty;
};
#endif

// Given a target type and storage of an array handle, provides a list this array handle plus all
// array handles that can be cast to the target type wrapped in an ArrayHandleCast that does so.
template <typename TargetT, typename Storage>
struct AllCastArraysForStorage
{
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
  VTKM_IS_LIST(StorageList);
  using listOfLists =
    vtkm::ListTransform<StorageList, AllCastArraysTransform<TargetT>::template Transform>;
  using type = vtkm::ListApply<listOfLists, vtkm::ListAppend>;
};

} // detail

template <typename TargetT, typename StorageList>
using ArrayHandleMultiplexerForStorageList = vtkm::cont::ArrayHandleMultiplexerFromList<
  typename detail::AllCastArraysForStorageList<TargetT, StorageList>::type>;

} // namespace internal

//-----------------------------------------------------------------------------
/// \brief Get an array from a `Field` that is not the active field.
///
/// Use this form for getting a `Field` when you don't know the type and it is not
/// (necessarily) the "active" field of the filter. It is generally used for arrays
/// passed to the `DoMapField` method of filters.
///
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::UncertainArrayHandle<
  typename std::conditional<
    std::is_same<typename DerivedPolicy::FieldTypeList, vtkm::ListUniversal>::value,
    VTKM_DEFAULT_TYPE_LIST,
    typename DerivedPolicy::FieldTypeList>::type,
  typename DerivedPolicy::StorageList>
ApplyPolicyFieldNotActive(const vtkm::cont::Field& field, vtkm::filter::PolicyBase<DerivedPolicy>)
{
  // Policies are on their way out, but until they are we want to respect them. In the mean
  // time, respect the policy if it is defined.
  using TypeList = typename std::conditional<
    std::is_same<typename DerivedPolicy::FieldTypeList, vtkm::ListUniversal>::value,
    VTKM_DEFAULT_TYPE_LIST,
    typename DerivedPolicy::FieldTypeList>::type;
  return field.GetData().ResetTypes(TypeList{}, typename DerivedPolicy::StorageList{});
}

//-----------------------------------------------------------------------------
/// \brief Get an `ArrayHandle` of a specific type from a `Field`.
///
/// Use this form of `ApplyPolicy` when you know what the value type of a field is or
/// (more likely) there is a type you are going to cast it to anyway.
///
template <typename T, typename DerivedPolicy, typename FilterType>
VTKM_CONT internal::ArrayHandleMultiplexerForStorageList<
  T,
  vtkm::ListAppend<typename vtkm::filter::FilterTraits<FilterType>::AdditionalFieldStorage,
                   typename DerivedPolicy::StorageList>>
ApplyPolicyFieldOfType(const vtkm::cont::Field& field,
                       vtkm::filter::PolicyBase<DerivedPolicy>,
                       const FilterType&)
{
  using ArrayHandleMultiplexerType = internal::ArrayHandleMultiplexerForStorageList<
    T,
    vtkm::ListAppend<typename FilterType::AdditionalFieldStorage,
                     typename DerivedPolicy::StorageList>>;
  return field.GetData().AsArrayHandle<ArrayHandleMultiplexerType>();
}

//-----------------------------------------------------------------------------
/// \brief Get an array from a `Field` that follows the types of an active field.
///
/// Use this form for getting a `Field` to build the types that are appropriate for
/// the active field of this filter.
///
template <typename DerivedPolicy, typename FilterType>
VTKM_CONT vtkm::cont::UncertainArrayHandle<
  typename vtkm::filter::DeduceFilterFieldTypes<
    DerivedPolicy,
    typename vtkm::filter::FilterTraits<FilterType>::InputFieldTypeList>::TypeList,
  typename vtkm::filter::DeduceFilterFieldStorage<
    DerivedPolicy,
    typename vtkm::filter::FilterTraits<FilterType>::AdditionalFieldStorage>::StorageList>
ApplyPolicyFieldActive(const vtkm::cont::Field& field,
                       vtkm::filter::PolicyBase<DerivedPolicy>,
                       vtkm::filter::FilterTraits<FilterType>)
{
  using FilterTypes = typename vtkm::filter::FilterTraits<FilterType>::InputFieldTypeList;
  using TypeList =
    typename vtkm::filter::DeduceFilterFieldTypes<DerivedPolicy, FilterTypes>::TypeList;
  using FilterStorage = typename vtkm::filter::FilterTraits<FilterType>::AdditionalFieldStorage;
  using StorageList =
    typename vtkm::filter::DeduceFilterFieldStorage<DerivedPolicy, FilterStorage>::StorageList;
  return field.GetData().ResetTypes(TypeList{}, StorageList{});
}

//-----------------------------------------------------------------------------
/// \brief Ge a cell set from a `UnknownCellSet` object.
///
/// Adjusts the types of `CellSet`s to support those types specified in a policy.
///
template <typename DerivedPolicy, typename DerivedFilter>
VTKM_CONT vtkm::cont::UncertainCellSet<vtkm::ListAppend<typename DerivedFilter::SupportedCellSets,
                                                        typename DerivedPolicy::AllCellSetList>>
ApplyPolicyCellSet(const vtkm::cont::UnknownCellSet& cellset,
                   vtkm::filter::PolicyBase<DerivedPolicy>,
                   const vtkm::filter::Filter<DerivedFilter>&)
{
  using CellSetList = vtkm::ListAppend<typename DerivedFilter::SupportedCellSets,
                                       typename DerivedPolicy::AllCellSetList>;
  return cellset.ResetCellSetList(CellSetList());
}

template <typename DerivedPolicy>
VTKM_DEPRECATED(1.6, "ApplyPolicyCellSet now takes the filter as an argument.")
VTKM_CONT vtkm::cont::UncertainCellSet<typename DerivedPolicy::AllCellSetList> ApplyPolicyCellSet(
  const vtkm::cont::UnknownCellSet& cellset,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  using CellSetList = typename DerivedPolicy::AllCellSetList;
  return cellset.ResetCellSetList(CellSetList());
}

//-----------------------------------------------------------------------------
/// \brief Get a structured cell set from a `UnknownCellSet` object.
///
/// Adjusts the types of `CellSet`s to support those structured cell set types
/// specified in a policy.
///
template <typename DerivedPolicy, typename DerivedFilter>
VTKM_CONT
  vtkm::cont::UncertainCellSet<vtkm::ListAppend<typename DerivedFilter::SupportedStructuredCellSets,
                                                typename DerivedPolicy::StructuredCellSetList>>
  ApplyPolicyCellSetStructured(const vtkm::cont::UnknownCellSet& cellset,
                               vtkm::filter::PolicyBase<DerivedPolicy>,
                               const vtkm::filter::Filter<DerivedFilter>&)
{
  using CellSetList = vtkm::ListAppend<typename DerivedFilter::SupportedStructuredCellSets,
                                       typename DerivedPolicy::StructuredCellSetList>;
  return cellset.ResetCellSetList(CellSetList());
}

template <typename DerivedPolicy>
VTKM_DEPRECATED(1.6, "ApplyPolicyCellSetStructured now takes the filter as an argument.")
VTKM_CONT vtkm::cont::
  UncertainCellSet<typename DerivedPolicy::StructuredCellSetList> ApplyPolicyCellSetStructured(
    const vtkm::cont::UnknownCellSet& cellset,
    vtkm::filter::PolicyBase<DerivedPolicy>)
{
  using CellSetList = typename DerivedPolicy::StructuredCellSetList;
  return cellset.ResetCellSetList(CellSetList());
}

//-----------------------------------------------------------------------------
/// \brief Get an unstructured cell set from a `UnknownCellSet` object.
///
/// Adjusts the types of `CellSet`s to support those unstructured cell set types
/// specified in a policy.
///
template <typename DerivedPolicy, typename DerivedFilter>
VTKM_CONT vtkm::cont::UncertainCellSet<
  vtkm::ListAppend<typename DerivedFilter::SupportedUnstructuredCellSets,
                   typename DerivedPolicy::UnstructuredCellSetList>>
ApplyPolicyCellSetUnstructured(const vtkm::cont::UnknownCellSet& cellset,
                               vtkm::filter::PolicyBase<DerivedPolicy>,
                               const vtkm::filter::Filter<DerivedFilter>&)
{
  using CellSetList = vtkm::ListAppend<typename DerivedFilter::SupportedUnstructuredCellSets,
                                       typename DerivedPolicy::UnstructuredCellSetList>;
  return cellset.ResetCellSetList(CellSetList());
}

template <typename DerivedPolicy>
VTKM_DEPRECATED(1.6, "ApplyPolicyCellSetUnstructured now takes the filter as an argument.")
VTKM_CONT vtkm::cont::
  UncertainCellSet<typename DerivedPolicy::UnstructuredCellSetList> ApplyPolicyCellSetUnstructured(
    const vtkm::cont::UnknownCellSet& cellset,
    vtkm::filter::PolicyBase<DerivedPolicy>)
{
  using CellSetList = typename DerivedPolicy::UnstructuredCellSetList;
  return cellset.ResetCellSetList(CellSetList());
}

//-----------------------------------------------------------------------------
VTKM_DEPRECATED_SUPPRESS_BEGIN
template <typename DerivedPolicy>
VTKM_DEPRECATED(1.6, "MakeSerializableField is no longer needed.")
VTKM_CONT
  vtkm::cont::SerializableField<typename DerivedPolicy::FieldTypeList> MakeSerializableField(
    vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return {};
}

template <typename DerivedPolicy>
VTKM_DEPRECATED(1.6, "MakeSerializableField is no longer needed.")
VTKM_CONT
  vtkm::cont::SerializableField<typename DerivedPolicy::FieldTypeList> MakeSerializableField(
    const vtkm::cont::Field& field,
    vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return vtkm::cont::SerializableField<typename DerivedPolicy::FieldTypeList>{ field };
}
VTKM_DEPRECATED_SUPPRESS_END

template <typename DerivedPolicy>
VTKM_DEPRECATED(1.6, "MakeSerializableDataSet now takes the filter as an argument.")
VTKM_CONT vtkm::cont::SerializableDataSet<
  typename DerivedPolicy::FieldTypeList,
  typename DerivedPolicy::AllCellSetList> MakeSerializableDataSet(vtkm::filter::
                                                                    PolicyBase<DerivedPolicy>)
{
  return {};
}

template <typename DerivedPolicy, typename DerivedFilter>
VTKM_CONT
  vtkm::cont::SerializableDataSet<typename DerivedPolicy::FieldTypeList,
                                  vtkm::ListAppend<typename DerivedFilter::SupportedCellSets,
                                                   typename DerivedPolicy::AllCellSetList>>
  MakeSerializableDataSet(vtkm::filter::PolicyBase<DerivedPolicy>,
                          const vtkm::filter::Filter<DerivedFilter>&)
{
  return {};
}

template <typename DerivedFilter>
VTKM_CONT
  vtkm::cont::SerializableDataSet<VTKM_DEFAULT_TYPE_LIST, typename DerivedFilter::SupportedCellSets>
  MakeSerializableDataSet(const vtkm::filter::Filter<DerivedFilter>&)
{
  return {};
}

template <typename DerivedPolicy>
VTKM_DEPRECATED(1.6, "MakeSerializableDataSet now takes the filter as an argument.")
VTKM_CONT vtkm::cont::SerializableDataSet<
  typename DerivedPolicy::FieldTypeList,
  typename DerivedPolicy::AllCellSetList> MakeSerializableDataSet(const vtkm::cont::DataSet&
                                                                    dataset,
                                                                  vtkm::filter::PolicyBase<
                                                                    DerivedPolicy>)
{
  return vtkm::cont::SerializableDataSet<typename DerivedPolicy::FieldTypeList,
                                         typename DerivedPolicy::AllCellSetList>{ dataset };
}

template <typename DerivedPolicy, typename DerivedFilter>
VTKM_CONT
  vtkm::cont::SerializableDataSet<typename DerivedPolicy::FieldTypeList,
                                  vtkm::ListAppend<typename DerivedFilter::SupportedCellSets,
                                                   typename DerivedPolicy::AllCellSetList>>
  MakeSerializableDataSet(const vtkm::cont::DataSet& dataset,
                          vtkm::filter::PolicyBase<DerivedPolicy>,
                          const vtkm::filter::Filter<DerivedFilter>&)
{
  return vtkm::cont::SerializableDataSet<typename DerivedPolicy::FieldTypeList,
                                         vtkm::ListAppend<typename DerivedFilter::SupportedCellSets,
                                                          typename DerivedPolicy::AllCellSetList>>{
    dataset
  };
}

template <typename DerivedFilter>
VTKM_CONT
  vtkm::cont::SerializableDataSet<VTKM_DEFAULT_TYPE_LIST, typename DerivedFilter::SupportedCellSets>
  MakeSerializableDataSet(const vtkm::cont::DataSet& dataset,
                          const vtkm::filter::Filter<DerivedFilter>&)
{
  return vtkm::cont::SerializableDataSet<VTKM_DEFAULT_TYPE_LIST,
                                         typename DerivedFilter::SupportedCellSets>{ dataset };
}
}
} // vtkm::filter

#endif //vtk_m_filter_PolicyBase_h
