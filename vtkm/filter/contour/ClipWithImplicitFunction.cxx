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
  if (field.IsFieldPoint())
  {
    auto resolve = [&](auto concrete) {
      using T = typename decltype(concrete)::ValueType;
      vtkm::cont::ArrayHandle<T> outputArray;
      outputArray = Worklet.ProcessPointField(concrete);
      result.AddPointField(field.GetName(), outputArray);
    };

    auto inputArray = field.GetData();
    inputArray
      .CastAndCallForTypesWithFloatFallback<vtkm::TypeListScalarAll, VTKM_DEFAULT_STORAGE_LIST>(
        resolve);
    return true;
  }
  else if (field.IsFieldCell())
  {
    // Use the precompiled field permutation function.
    vtkm::cont::ArrayHandle<vtkm::Id> permutation = Worklet.GetCellMapOutputToInput();
    return vtkm::filter::MapFieldPermutation(field, permutation, result);
  }
  else if (field.IsFieldGlobal())
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
  //get the cells and coordinates of the dataset
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();

  const vtkm::cont::CoordinateSystem& inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::worklet::Clip Worklet;

  vtkm::cont::CellSetExplicit<> outputCellSet =
    Worklet.Run(cells, this->Function, inputCoords, this->Invert);

  //create the output data
  vtkm::cont::DataSet output;
  output.SetCellSet(outputCellSet);

  // compute output coordinates
  for (vtkm::IdComponent coordSystemId = 0; coordSystemId < input.GetNumberOfCoordinateSystems();
       ++coordSystemId)
  {
    const vtkm::cont::CoordinateSystem& coords = input.GetCoordinateSystem(coordSystemId);
    coords.GetData().CastAndCall(
      ClipWithImplicitFunctionProcessCoords{}, coords.GetName(), Worklet, output);
  }

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, Worklet); };
  MapFieldsOntoOutput(input, output, mapper);

  return output;
}
} // namespace contour
} // namespace filter
} // namespace vtkm
