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
  if (field.IsFieldPoint())
  {
    auto array = field.GetData();
    auto functor = [&](auto concrete) {
      using T = typename decltype(concrete)::ValueType;
      vtkm::cont::ArrayHandle<T> output;
      output = worklet.ProcessPointField(concrete);
      result.template AddPointField(field.GetName(), output);
    };

    array.CastAndCallForTypesWithFloatFallback<vtkm::TypeListScalarAll, VTKM_DEFAULT_STORAGE_LIST>(
      functor);
    return true;
  }
  else if (field.IsFieldCell())
  {
    // Use the precompiled field permutation function.
    vtkm::cont::ArrayHandle<vtkm::Id> permutation = worklet.GetCellMapOutputToInput();
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
} // anonymous

namespace contour
{
//-----------------------------------------------------------------------------
vtkm::cont::DataSet ClipWithField::DoExecute(const vtkm::cont::DataSet& input)
{
  auto field = this->GetFieldFromDataSet(input);
  if (!field.IsFieldPoint())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  vtkm::worklet::Clip Worklet;

  //get the cells and coordinates of the dataset
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();

  const auto& inArray = this->GetFieldFromDataSet(input).GetData();
  vtkm::cont::DataSet output;

  auto ResolveFieldType = [&, this](auto concrete) {
    vtkm::cont::CellSetExplicit<> outputCellSet =
      Worklet.Run(cells, concrete, this->ClipValue, this->Invert);

    output.SetCellSet(outputCellSet);

    // Compute the new boundary points and add them to the output:
    for (vtkm::IdComponent coordSystemId = 0; coordSystemId < input.GetNumberOfCoordinateSystems();
         ++coordSystemId)
    {
      const vtkm::cont::CoordinateSystem& coords = input.GetCoordinateSystem(coordSystemId);
      coords.GetData().CastAndCall(ClipWithFieldProcessCoords{}, coords.GetName(), Worklet, output);
    }
  };

  inArray.CastAndCallForTypesWithFloatFallback<vtkm::TypeListScalarAll, VTKM_DEFAULT_STORAGE_LIST>(
    ResolveFieldType);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, Worklet); };
  MapFieldsOntoOutput(input, output, mapper);

  return output;
}
}
}
} // end namespace vtkm::filter
