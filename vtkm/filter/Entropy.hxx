//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/FieldEntropy.h>

#include <vtkm/filter/internal/CreateResult.h>

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

  return internal::CreateResult(inDataSet,
                                entropy,
                                this->GetOutputFieldName(),
                                fieldMetadata.GetAssociation(),
                                fieldMetadata.GetCellSetName());
}
}
} // namespace vtkm::filter
