//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ErrorFilterExecution.h>

#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/contour/ClipWithField.h>
#include <vtkm/filter/contour/worklet/Clip.h>

namespace vtkm
{
namespace filter
{
namespace contour
{
namespace
{

bool DoMapField(vtkm::cont::DataSet& result,
                const vtkm::cont::Field& field,
                vtkm::worklet::Clip& worklet)
{
  if (field.IsPointField())
  {
    vtkm::cont::UnknownArrayHandle inputArray = field.GetData();
    vtkm::cont::UnknownArrayHandle outputArray = inputArray.NewInstanceBasic();

    auto resolve = [&](const auto& concreteIn) {
      // use std::decay to remove const ref from the decltype of concrete.
      using BaseT = typename std::decay_t<decltype(concreteIn)>::ValueType::ComponentType;
      auto concreteOut = outputArray.ExtractArrayFromComponents<BaseT>();
      worklet.ProcessPointField(concreteIn, concreteOut);
    };

    inputArray.CastAndCallWithExtractedArray(resolve);
    result.AddPointField(field.GetName(), outputArray);
    return true;
  }
  else if (field.IsCellField())
  {
    // Use the precompiled field permutation function.
    vtkm::cont::ArrayHandle<vtkm::Id> permutation = worklet.GetCellMapOutputToInput();
    return vtkm::filter::MapFieldPermutation(field, permutation, result);
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}
} // anonymous

//-----------------------------------------------------------------------------
vtkm::cont::DataSet ClipWithField::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);
  if (!field.IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  vtkm::worklet::Clip worklet;

  const vtkm::cont::UnknownCellSet& inputCellSet = input.GetCellSet();
  vtkm::cont::CellSetExplicit<> outputCellSet;

  auto resolveFieldType = [&](const auto& concrete) {
    outputCellSet = worklet.Run(inputCellSet, concrete, this->ClipValue, this->Invert);
  };
  this->CastAndCallScalarField(this->GetFieldFromDataSet(input).GetData(), resolveFieldType);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, outputCellSet, mapper);
}
} // namespace contour
} // namespace filter
} // namespace vtkm
