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

#ifndef vtk_m_filter_PolicyBase_h
#define vtk_m_filter_PolicyBase_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/Field.h>

#include <vtkm/filter/FilterTraits.h>

namespace vtkm {
namespace filter {

template<typename Derived>
class PolicyBase
{


};


//-----------------------------------------------------------------------------
template<typename DerivedPolicy>
VTKM_CONT_EXPORT
vtkm::cont::DynamicArrayHandleBase<
                                  typename DerivedPolicy::FieldTypeList,
                                  typename DerivedPolicy::FieldStorageList
                                  >
ApplyPolicy(const vtkm::cont::Field& field,
            const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  typedef typename DerivedPolicy::FieldTypeList TypeList;
  typedef typename DerivedPolicy::FieldStorageList StorageList;
  return field.GetData().ResetTypeAndStorageLists(TypeList(),StorageList());
}

//-----------------------------------------------------------------------------
template<typename DerivedPolicy, typename FilterType>
VTKM_CONT_EXPORT
vtkm::cont::DynamicArrayHandleBase<
                                  typename vtkm::filter::FilterTraits<FilterType>::InputFieldTypeList,
                                  typename DerivedPolicy::FieldStorageList
                                  >
ApplyPolicy(const vtkm::cont::Field& field,
            const vtkm::filter::PolicyBase<DerivedPolicy>&,
            const vtkm::filter::FilterTraits<FilterType>&)
{
  //todo: we need to intersect the policy field type list and the
  //filter traits to the get smallest set of valid types
  typedef typename vtkm::filter::FilterTraits<FilterType>::InputFieldTypeList TypeList;
  // typedef typename DerivedPolicy::FieldTypeList TypeList;
  typedef typename DerivedPolicy::FieldStorageList StorageList;
  return field.GetData().ResetTypeAndStorageLists(TypeList(),StorageList());
}

//-----------------------------------------------------------------------------
template<typename DerivedPolicy>
VTKM_CONT_EXPORT
vtkm::cont::DynamicArrayHandleBase<
                                  typename DerivedPolicy::CoordinateTypeList,
                                  typename DerivedPolicy::CoordinateStorageList
                                  >
ApplyPolicy(const vtkm::cont::CoordinateSystem& coordinates,
            const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  typedef typename DerivedPolicy::CoordinateTypeList TypeList;
  typedef typename DerivedPolicy::CoordinateStorageList StorageList;
  return coordinates.GetData().ResetTypeAndStorageLists(TypeList(),StorageList());
}

//-----------------------------------------------------------------------------
template<typename DerivedPolicy, typename FilterType>
VTKM_CONT_EXPORT
vtkm::cont::DynamicArrayHandleBase<
                                  typename vtkm::filter::FilterTraits<FilterType>::InputFieldTypeList,
                                  typename DerivedPolicy::CoordinateStorageList
                                  >
ApplyPolicy(const vtkm::cont::CoordinateSystem& coordinates,
            const vtkm::filter::PolicyBase<DerivedPolicy>&,
            const vtkm::filter::FilterTraits<FilterType>&)
{
  //todo: we need to intersect the policy field type list and the
  //filter traits to the get smallest set of valid types
  typedef typename DerivedPolicy::CoordinateTypeList TypeList;
  typedef typename DerivedPolicy::CoordinateStorageList StorageList;
  return coordinates.GetData().ResetTypeAndStorageLists(TypeList(),StorageList());
}

//-----------------------------------------------------------------------------
template<typename DerivedPolicy>
VTKM_CONT_EXPORT
vtkm::cont::DynamicCellSetBase< typename DerivedPolicy::CellSetList >
ApplyPolicy(const vtkm::cont::DynamicCellSet& cellset,
            const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  typedef typename DerivedPolicy::CellSetList CellSetList;
  return cellset.ResetCellSetList(CellSetList());
}

}
}

#endif //vtk_m_filter_PolicyBase_h
