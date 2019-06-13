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

  using StructuredCellSetList = vtkm::cont::CellSetListTagStructured;
  using UnstructuredCellSetList = vtkm::cont::CellSetListTagUnstructured;
  using AllCellSetList = VTKM_DEFAULT_CELL_SET_LIST_TAG;
};

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
template <typename DerivedPolicy, typename FilterType, typename FieldTag>
VTKM_CONT vtkm::cont::VariantArrayHandleBase<
  typename vtkm::filter::DeduceFilterFieldTypes<DerivedPolicy, FilterType, FieldTag>::TypeList>
ApplyPolicy(const vtkm::cont::Field& field,
            const vtkm::filter::PolicyBase<DerivedPolicy>&,
            const vtkm::filter::FilterTraits<FilterType, FieldTag>&)
{
  using TypeList =
    typename vtkm::filter::DeduceFilterFieldTypes<DerivedPolicy, FilterType, FieldTag>::TypeList;
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
