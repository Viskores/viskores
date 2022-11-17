//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/field_transform/PointTransform.h>
#include <vtkm/filter/field_transform/worklet/PointTransform.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
//-----------------------------------------------------------------------------
VTKM_CONT PointTransform::PointTransform()
{
  this->SetOutputFieldName("transform");
  this->SetUseCoordinateSystemAsField(true);
}

//-----------------------------------------------------------------------------
VTKM_CONT void PointTransform::SetChangeCoordinateSystem(bool flag)
{
  this->ChangeCoordinateSystem = flag;
}

//-----------------------------------------------------------------------------
VTKM_CONT bool PointTransform::GetChangeCoordinateSystem() const
{
  return this->ChangeCoordinateSystem;
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet PointTransform::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::UnknownArrayHandle outArray;

  auto resolveType = [&](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    vtkm::cont::ArrayHandle<T> result;
    this->Invoke(vtkm::worklet::PointTransform{ this->matrix }, concrete, result);
    outArray = result;
  };
  const auto& field = this->GetFieldFromDataSet(inDataSet);
  this->CastAndCallVecField<3>(field, resolveType);

  auto passMapper = [](auto& d, const auto& f) { d.AddField(f); };
  vtkm::cont::DataSet outData = this->CreateResultCoordinateSystem(
    inDataSet, inDataSet.GetCellSet(), this->GetOutputFieldName(), outArray, passMapper);

  if (this->GetChangeCoordinateSystem())
  {
    vtkm::Id coordIndex =
      this->GetUseCoordinateSystemAsField() ? this->GetActiveCoordinateSystemIndex() : 0;
    outData.GetCoordinateSystem(coordIndex).SetData(outArray);
  }

  return outData;
}
}
}
} // namespace vtkm::filter
