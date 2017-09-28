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

#ifndef vtk_m_filter_PolicyBase_h
#define vtk_m_filter_PolicyBase_h

#include <vtkm/TypeListTag.h>

#include <vtkm/cont/CellSetListTag.h>
#include <vtkm/cont/CoordinateSystem.h>
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
  typedef VTKM_DEFAULT_TYPE_LIST_TAG FieldTypeList;
  typedef VTKM_DEFAULT_STORAGE_LIST_TAG FieldStorageList;

  typedef vtkm::cont::CellSetListTagStructured StructuredCellSetList;
  typedef vtkm::cont::CellSetListTagUnstructured UnstructuredCellSetList;
  typedef VTKM_DEFAULT_CELL_SET_LIST_TAG AllCellSetList;

  typedef VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG CoordinateTypeList;
  typedef VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG CoordinateStorageList;

  // List of backends to try in sequence (if one fails, the next is attempted).
  typedef VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG DeviceAdapterList;
};

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::DynamicArrayHandleBase<typename DerivedPolicy::FieldTypeList,
                                             typename DerivedPolicy::FieldStorageList>
ApplyPolicy(const vtkm::cont::Field& field, const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  typedef typename DerivedPolicy::FieldTypeList TypeList;
  typedef typename DerivedPolicy::FieldStorageList StorageList;
  return field.GetData().ResetTypeAndStorageLists(TypeList(), StorageList());
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy, typename FilterType>
VTKM_CONT vtkm::cont::DynamicArrayHandleBase<
  typename vtkm::filter::DeduceFilterFieldTypes<DerivedPolicy, FilterType>::TypeList,
  typename DerivedPolicy::FieldStorageList>
ApplyPolicy(const vtkm::cont::Field& field,
            const vtkm::filter::PolicyBase<DerivedPolicy>&,
            const vtkm::filter::FilterTraits<FilterType>&)
{
  typedef
    typename vtkm::filter::DeduceFilterFieldTypes<DerivedPolicy, FilterType>::TypeList TypeList;

  typedef typename DerivedPolicy::FieldStorageList StorageList;
  return field.GetData().ResetTypeAndStorageLists(TypeList(), StorageList());
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::DynamicArrayHandleBase<typename DerivedPolicy::CoordinateTypeList,
                                             typename DerivedPolicy::CoordinateStorageList>
ApplyPolicy(const vtkm::cont::CoordinateSystem& coordinates,
            const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  typedef typename DerivedPolicy::CoordinateTypeList TypeList;
  typedef typename DerivedPolicy::CoordinateStorageList StorageList;
  return coordinates.GetData().ResetTypeAndStorageLists(TypeList(), StorageList());
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy, typename FilterType>
VTKM_CONT vtkm::cont::DynamicArrayHandleBase<typename DerivedPolicy::CoordinateTypeList,
                                             typename DerivedPolicy::CoordinateStorageList>
ApplyPolicy(const vtkm::cont::CoordinateSystem& coordinates,
            const vtkm::filter::PolicyBase<DerivedPolicy>&,
            const vtkm::filter::FilterTraits<FilterType>&)
{
  //todo: we need to intersect the policy field type list and the
  //filter traits to the get smallest set of valid types
  typedef typename DerivedPolicy::CoordinateTypeList TypeList;
  typedef typename DerivedPolicy::CoordinateStorageList StorageList;
  return coordinates.GetData().ResetTypeAndStorageLists(TypeList(), StorageList());
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::DynamicCellSetBase<typename DerivedPolicy::AllCellSetList> ApplyPolicy(
  const vtkm::cont::DynamicCellSet& cellset,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  typedef typename DerivedPolicy::AllCellSetList CellSetList;
  return cellset.ResetCellSetList(CellSetList());
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::DynamicCellSetBase<typename DerivedPolicy::StructuredCellSetList>
ApplyPolicyStructured(const vtkm::cont::DynamicCellSet& cellset,
                      const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  typedef typename DerivedPolicy::StructuredCellSetList CellSetList;
  return cellset.ResetCellSetList(CellSetList());
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
VTKM_CONT vtkm::cont::DynamicCellSetBase<typename DerivedPolicy::UnstructuredCellSetList>
ApplyPolicyUnstructured(const vtkm::cont::DynamicCellSet& cellset,
                        const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  typedef typename DerivedPolicy::UnstructuredCellSetList CellSetList;
  return cellset.ResetCellSetList(CellSetList());
}
}
}

#endif //vtk_m_filter_PolicyBase_h
