//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleCast.h>

#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT CrossProduct::CrossProduct()
  : vtkm::filter::FilterField<CrossProduct>()
  , SecondaryFieldName()
  , SecondaryFieldAssociation(vtkm::cont::Field::Association::ANY)
  , UseCoordinateSystemAsSecondaryField(false)
  , SecondaryCoordinateSystemIndex(0)
{
  this->SetOutputFieldName("crossproduct");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet CrossProduct::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& primary,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  vtkm::cont::Field secondaryField;
  if (this->UseCoordinateSystemAsSecondaryField)
  {
    secondaryField = inDataSet.GetCoordinateSystem(this->GetSecondaryCoordinateSystemIndex());
  }
  else
  {
    secondaryField = inDataSet.GetField(this->SecondaryFieldName, this->SecondaryFieldAssociation);
  }
  auto secondary = vtkm::filter::ApplyPolicy<vtkm::Vec<T, 3>>(secondaryField, policy, *this);

  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> output;
  this->Invoke(vtkm::worklet::CrossProduct{}, primary, secondary, output);

  return CreateResult(inDataSet, output, this->GetOutputFieldName(), fieldMetadata);
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool CrossProduct::DoMapField(vtkm::cont::DataSet& result,
                                               const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                               const vtkm::filter::FieldMetadata& fieldMeta,
                                               vtkm::filter::PolicyBase<DerivedPolicy>)
{
  //we copy the input handle to the result dataset, reusing the metadata
  result.AddField(fieldMeta.AsField(input));
  return true;
}
}
} // namespace vtkm::filter
