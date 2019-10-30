//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Entropy_hxx
#define vtk_m_filter_Entropy_hxx

#include <vtkm/worklet/FieldEntropy.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Entropy::Entropy()
  : NumberOfBins(10)
{
  this->SetOutputFieldName("entropy");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet Entropy::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  vtkm::worklet::FieldEntropy worklet;

  vtkm::Float64 e = worklet.Run(field, this->NumberOfBins);

  //the entropy vector only contain one element, the entorpy of the input field
  vtkm::cont::ArrayHandle<vtkm::Float64> entropy;
  entropy.Allocate(1);
  entropy.GetPortalControl().Set(0, e);

  return CreateResult(inDataSet, entropy, this->GetOutputFieldName(), fieldMetadata);
}
}
} // namespace vtkm::filter

#endif
