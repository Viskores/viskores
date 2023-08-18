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
#include <vtkm/cont/UnknownCellSet.h>
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/contour/ClipWithImplicitFunction.h>
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

    auto resolve = [&](const auto& concrete) {
      // use std::decay to remove const ref from the decltype of concrete.
      using BaseT = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
      auto concreteOut = outputArray.ExtractArrayFromComponents<BaseT>();
      worklet.ProcessPointField(concrete, concreteOut);
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
} // anonymous namespace

//-----------------------------------------------------------------------------
vtkm::cont::DataSet ClipWithImplicitFunction::DoExecute(const vtkm::cont::DataSet& input)
{
  const vtkm::cont::UnknownCellSet& inputCellSet = input.GetCellSet();
  const vtkm::cont::CoordinateSystem& inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::worklet::Clip worklet;

  vtkm::cont::CellSetExplicit<> outputCellSet =
    worklet.Run(inputCellSet, this->Function, this->Offset, inputCoords, this->Invert);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, outputCellSet, mapper);
}
} // namespace contour
} // namespace filter
} // namespace vtkm
