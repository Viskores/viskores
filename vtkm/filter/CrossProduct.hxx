//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_CrossProduct_hxx
#define vtk_m_filter_CrossProduct_hxx

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
  auto secondary =
    vtkm::filter::ApplyPolicyFieldOfType<vtkm::Vec<T, 3>>(secondaryField, policy, *this);

  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> output;
  this->Invoke(vtkm::worklet::CrossProduct{}, primary, secondary, output);

  return CreateResult(inDataSet, output, this->GetOutputFieldName(), fieldMetadata);
}
}
} // namespace vtkm::filter

#endif
