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
inline VTKM_CONT void PointTransform::SetChangeCoordinateSystem(bool flag)
{
  this->ChangeCoordinateSystem = flag;
}

//-----------------------------------------------------------------------------
inline VTKM_CONT bool PointTransform::GetChangeCoordinateSystem() const
{
  return this->ChangeCoordinateSystem;
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet PointTransform::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  auto field = this->GetFieldFromDataSet(inDataSet);
  vtkm::cont::UnknownArrayHandle outArray;

  auto resolveType = [&, this](const auto& concrete) {
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    vtkm::cont::ArrayHandle<T> result;
    this->Invoke(vtkm::worklet::PointTransform{ this->matrix }, field, result);
    outArray = result;
  };
  this->CastAndCallVecField<3>(field.GetData(), resolveType);

  vtkm::cont::DataSet outData = this->CreateResultField(
    inDataSet, this->GetOutputFieldName(), field.GetAssociation(), outArray);

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
