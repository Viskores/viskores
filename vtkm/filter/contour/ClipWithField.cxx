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
struct ClipWithFieldProcessCoords
{
  template <typename T, typename Storage>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, Storage>& inCoords,
                            const std::string& coordsName,
                            const vtkm::worklet::Clip& worklet,
                            vtkm::cont::DataSet& output) const
  {
    vtkm::cont::ArrayHandle<T> outArray = worklet.ProcessPointField(inCoords);
    vtkm::cont::CoordinateSystem outCoords(coordsName, outArray);
    output.AddCoordinateSystem(outCoords);
  }
};

bool DoMapField(vtkm::cont::DataSet& result,
                const vtkm::cont::Field& field,
                vtkm::worklet::Clip& worklet)
{
  if (field.IsPointField())
  {
    auto resolve = [&](const auto& concrete) {
      // use std::decay to remove const ref from the decltype of concrete.
      using T = typename std::decay_t<decltype(concrete)>::ValueType;
      vtkm::cont::ArrayHandle<T> outputArray;
      outputArray = worklet.ProcessPointField(concrete);
      result.AddPointField(field.GetName(), outputArray);
    };

    field.GetData()
      .CastAndCallForTypesWithFloatFallback<VTKM_DEFAULT_TYPE_LIST, VTKM_DEFAULT_STORAGE_LIST>(
        resolve);
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
  vtkm::cont::DataSet output = this->CreateResult(input, outputCellSet, mapper);

  // Compute the new boundary points and add them to the output:
  for (vtkm::IdComponent coordSystemId = 0; coordSystemId < input.GetNumberOfCoordinateSystems();
       ++coordSystemId)
  {
    const vtkm::cont::CoordinateSystem& coords = input.GetCoordinateSystem(coordSystemId);
    coords.GetData().CastAndCall(ClipWithFieldProcessCoords{}, coords.GetName(), worklet, output);
  }

  return output;
}
} // namespace contour
} // namespace filter
} // namespace vtkm
