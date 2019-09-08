//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT WarpVector::WarpVector(vtkm::FloatDefault scale)
  : vtkm::filter::FilterField<WarpVector>()
  , Worklet()
  , VectorFieldName("normal")
  , VectorFieldAssociation(vtkm::cont::Field::Association::ANY)
  , Scale(scale)
{
  this->SetOutputFieldName("warpvector");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet WarpVector::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  using vecType = vtkm::Vec<T, 3>;
  vtkm::cont::Field vectorF =
    inDataSet.GetField(this->VectorFieldName, this->VectorFieldAssociation);
  vtkm::cont::ArrayHandle<vecType> result;
  this->Worklet.Run(
    field, vtkm::filter::ApplyPolicy<vecType>(vectorF, policy, *this), this->Scale, result);

  return CreateResult(inDataSet, result, this->GetOutputFieldName(), fieldMetadata);
}
}
}
