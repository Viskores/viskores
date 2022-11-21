//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/filter/field_transform/CylindricalCoordinateTransform.h>
#include <vtkm/filter/field_transform/worklet/CoordinateSystemTransform.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{

CylindricalCoordinateTransform::CylindricalCoordinateTransform()
{
  this->SetUseCoordinateSystemAsField(true);
}

vtkm::cont::DataSet CylindricalCoordinateTransform::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::UnknownArrayHandle outArray;

  const vtkm::cont::Field& inField = this->GetFieldFromDataSet(inDataSet);
  if (!inField.IsPointField())
  {
    throw vtkm::cont::ErrorBadValue("CylindricalCoordinateTransform only applies to point data.");
  }

  auto resolveType = [&](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    vtkm::cont::ArrayHandle<T> result;
    if (this->CartesianToCylindrical)
      this->Invoke(vtkm::worklet::CarToCyl{}, concrete, result);
    else
      this->Invoke(vtkm::worklet::CylToCar{}, concrete, result);
    outArray = result;
  };
  this->CastAndCallVecField<3>(inField, resolveType);

  std::string coordinateName = this->GetOutputFieldName();
  if (coordinateName.empty())
  {
    coordinateName = inField.GetName();
  }

  vtkm::cont::DataSet outDataSet = this->CreateResultCoordinateSystem(
    inDataSet,
    inDataSet.GetCellSet(),
    vtkm::cont::CoordinateSystem(coordinateName, outArray),
    [](vtkm::cont::DataSet& out, const vtkm::cont::Field& fieldToPass) {
      out.AddField(fieldToPass);
    });
  return outDataSet;
}

} // namespace field_transform
} // namespace filter
} // namespace vtkm
