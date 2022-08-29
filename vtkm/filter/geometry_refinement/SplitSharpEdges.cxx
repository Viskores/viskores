//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/UnknownCellSet.h>
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/geometry_refinement/SplitSharpEdges.h>
#include <vtkm/filter/geometry_refinement/worklet/SplitSharpEdges.h>

namespace vtkm
{
namespace filter
{
namespace
{
VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& field,
                          const vtkm::worklet::SplitSharpEdges& worklet)
{
  if (field.IsPointField())
  {
    return vtkm::filter::MapFieldPermutation(field, worklet.GetNewPointsIdArray(), result);
  }
  else if (field.IsCellField() || field.IsWholeDataSetField())
  {
    result.AddField(field); // pass through
    return true;
  }
  else
  {
    return false;
  }
}
} // anonymous namespace

namespace geometry_refinement
{
//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet SplitSharpEdges::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);
  const vtkm::cont::UnknownCellSet& inCellSet = input.GetCellSet();
  const auto& oldCoords = input.GetCoordinateSystem().GetDataAsMultiplexer();

  vtkm::cont::ArrayHandle<vtkm::Vec3f> newCoords;
  vtkm::cont::CellSetExplicit<> newCellset;
  vtkm::worklet::SplitSharpEdges worklet;
  this->CastAndCallVecField<3>(field, [&](const auto& concrete) {
    worklet.Run(inCellSet, this->FeatureAngle, concrete, oldCoords, newCoords, newCellset);
  });

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  auto output = this->CreateResult(input, newCellset, mapper);
  output.AddCoordinateSystem(
    vtkm::cont::CoordinateSystem(input.GetCoordinateSystem().GetName(), newCoords));
  return output;
}

} // namespace geometry_refinement
} // namespace filter
} // namespace vtkm
