//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/geometry_refinement/Triangulate.h>
#include <vtkm/filter/geometry_refinement/worklet/Triangulate.h>

namespace
{
//-----------------------------------------------------------------------------
VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& field,
                          const vtkm::worklet::Triangulate& worklet)
{
  if (field.IsPointField())
  {
    // point data is copied as is because it was not collapsed
    result.AddField(field);
    return true;
  }
  else if (field.IsCellField())
  {
    // cell data must be scattered to the cells created per input cell
    vtkm::cont::ArrayHandle<vtkm::Id> permutation =
      worklet.GetOutCellScatter().GetOutputToInputMap();
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

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{
VTKM_CONT vtkm::cont::DataSet Triangulate::DoExecute(const vtkm::cont::DataSet& input)
{
  const vtkm::cont::UnknownCellSet& inCellSet = input.GetCellSet();

  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::worklet::Triangulate worklet;

  vtkm::cont::CastAndCall(inCellSet,
                          [&](const auto& concrete) { outCellSet = worklet.Run(concrete); });

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  // create the output dataset (without a CoordinateSystem).
  vtkm::cont::DataSet output = this->CreateResult(input, outCellSet, mapper);

  // We did not change the geometry of the input dataset at all. Just attach coordinate system
  // of input dataset to output dataset.
  for (vtkm::IdComponent coordSystemId = 0; coordSystemId < input.GetNumberOfCoordinateSystems();
       ++coordSystemId)
  {
    output.AddCoordinateSystem(input.GetCoordinateSystem(coordSystemId));
  }
  return output;
}
} // namespace geometry_refinement
} // namespace filter
} // namespace vtkm
