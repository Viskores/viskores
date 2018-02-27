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

#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT CrossProduct::CrossProduct()
  : vtkm::filter::FilterField<CrossProduct>()
  , Worklet()
  , SecondaryFieldName("")
{
  this->SetOutputFieldName("crossproduct");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::filter::Result CrossProduct::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter&)
{
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType> outArray;

  vtkm::worklet::DispatcherMapField<vtkm::worklet::CrossProduct, DeviceAdapter> dispatcher(
    this->Worklet);

  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType> secondaryField;
  try
  {
    vtkm::filter::ApplyPolicy(inDataSet.GetField(SecondaryFieldName), policy)
      .CopyTo(secondaryField);
  }
  catch (const vtkm::cont::Error&)
  {
    return vtkm::filter::Result();
  }

  dispatcher.Invoke(field, secondaryField, outArray);

  return vtkm::filter::Result(inDataSet,
                              outArray,
                              this->GetOutputFieldName(),
                              fieldMetadata.GetAssociation(),
                              fieldMetadata.GetCellSetName());
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool CrossProduct::DoMapField(vtkm::filter::Result& result,
                                               const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                               const vtkm::filter::FieldMetadata& fieldMeta,
                                               const vtkm::filter::PolicyBase<DerivedPolicy>&,
                                               DeviceAdapter)
{
  //we copy the input handle to the result dataset, reusing the metadata
  result.GetDataSet().AddField(fieldMeta.AsField(input));
  return true;
}
}
} // namespace vtkm::filter
