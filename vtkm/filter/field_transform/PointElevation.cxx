//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/field_transform/PointElevation.h>
#include <vtkm/filter/field_transform/worklet/PointElevation.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
//-----------------------------------------------------------------------------
VTKM_CONT PointElevation::PointElevation()
{
  this->SetOutputFieldName("elevation");
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet PointElevation::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::ArrayHandle<vtkm::Float64> outArray;

  auto resolveType = [&](const auto& concrete) {
    this->Invoke(
      vtkm::worklet::PointElevation{
        this->LowPoint, this->HighPoint, this->RangeLow, this->RangeHigh },
      concrete,
      outArray);
  };
  const auto& field = this->GetFieldFromDataSet(inDataSet);
  this->CastAndCallVecField<3>(field, resolveType);

  return this->CreateResultField(
    inDataSet, this->GetOutputFieldName(), field.GetAssociation(), outArray);
}
} // namespace field_transform
} // namespace filter
} // namespace vtkm
