//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/CreateResult.h>
#include <vtkm/filter/density_estimate/Entropy.h>
#include <vtkm/filter/density_estimate/worklet/FieldEntropy.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
//-----------------------------------------------------------------------------
VTKM_CONT Entropy::Entropy()

{
  this->SetOutputFieldName("entropy");
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet Entropy::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::worklet::FieldEntropy worklet;

  vtkm::Float64 e = 0;
  auto resolveType = [&](const auto& concrete) { e = worklet.Run(concrete, this->NumberOfBins); };
  const auto& fieldArray = this->GetFieldFromDataSet(inDataSet).GetData();
  fieldArray
    .CastAndCallForTypesWithFloatFallback<vtkm::TypeListScalarAll, VTKM_DEFAULT_STORAGE_LIST>(
      resolveType);

  //the entropy vector only contain one element, the entorpy of the input field
  vtkm::cont::ArrayHandle<vtkm::Float64> entropy;
  entropy.Allocate(1);
  entropy.WritePortal().Set(0, e);

  auto result = CreateResult(inDataSet,
                             vtkm::cont::Field{ this->GetOutputFieldName(),
                                                vtkm::cont::Field::Association::WHOLE_MESH,
                                                entropy });

  this->MapFieldsOntoOutput(inDataSet, result);

  return result;
}
} // namespace density_estimate
} // namespace filter
} // namespace vtkm
