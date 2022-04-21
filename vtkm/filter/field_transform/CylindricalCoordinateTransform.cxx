//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/field_transform/CylindricalCoordinateTransform.h>
#include <vtkm/filter/field_transform/worklet/CoordinateSystemTransform.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{

vtkm::cont::DataSet CylindricalCoordinateTransform::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::UnknownArrayHandle outArray;

  auto resolveType = [&](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    vtkm::cont::ArrayHandle<T> result;
    vtkm::worklet::CylindricalCoordinateTransform worklet{ this->CartesianToCylindrical };
    worklet.Run(concrete, result);
    outArray = result;
  };
  this->CastAndCallVecField<3>(this->GetFieldFromDataSet(inDataSet), resolveType);

  vtkm::cont::DataSet outDataSet =
    this->CreateResult(inDataSet,
                       inDataSet.GetCellSet(),
                       vtkm::cont::CoordinateSystem("coordinates", outArray),
                       [](vtkm::cont::DataSet& out, const vtkm::cont::Field& fieldToPass) {
                         out.AddField(fieldToPass);
                       });
  return outDataSet;
}

} // namespace field_transform
} // namespace filter
} // namespace vtkm
