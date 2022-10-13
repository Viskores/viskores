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

struct ClipWithImplicitFunctionProcessCoords
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
                const vtkm::worklet::Clip& Worklet)
{
  if (field.IsPointField())
  {
    auto resolve = [&](const auto& concrete) {
      // use std::decay to remove const ref from the decltype of concrete.
      using T = typename std::decay_t<decltype(concrete)>::ValueType;
      vtkm::cont::ArrayHandle<T> outputArray;
      outputArray = Worklet.ProcessPointField(concrete);
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
    vtkm::cont::ArrayHandle<vtkm::Id> permutation = Worklet.GetCellMapOutputToInput();
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
  vtkm::cont::DataSet output = this->CreateResult(input, outputCellSet, mapper);

  // compute output coordinates
  for (vtkm::IdComponent coordSystemId = 0; coordSystemId < input.GetNumberOfCoordinateSystems();
       ++coordSystemId)
  {
    const vtkm::cont::CoordinateSystem& coords = input.GetCoordinateSystem(coordSystemId);
    coords.GetData().CastAndCall(
      ClipWithImplicitFunctionProcessCoords{}, coords.GetName(), worklet, output);
  }

  return output;
}
} // namespace contour
} // namespace filter
} // namespace vtkm
