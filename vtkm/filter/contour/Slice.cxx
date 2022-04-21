//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopyDevice.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/filter/contour/Slice.h>

namespace vtkm
{
namespace filter
{
namespace contour
{
vtkm::cont::DataSet Slice::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& coords = input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::cont::DataSet result;
  auto impFuncEval =
    vtkm::ImplicitFunctionValueFunctor<vtkm::ImplicitFunctionGeneral>(this->Function);
  auto coordTransform =
    vtkm::cont::make_ArrayHandleTransform(coords.GetDataAsMultiplexer(), impFuncEval);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> sliceScalars;
  vtkm::cont::ArrayCopyDevice(coordTransform, sliceScalars);
  // input is a const, we can not AddField to it.
  vtkm::cont::DataSet clone = input;
  clone.AddField(vtkm::cont::make_FieldPoint("sliceScalars", sliceScalars));

  this->Contour::SetIsoValue(0.0);
  this->Contour::SetActiveField("sliceScalars");
  result = this->Contour::DoExecute(clone);

  return result;
}
} // namespace contour
} // namespace filter
} // namespace vtkm
