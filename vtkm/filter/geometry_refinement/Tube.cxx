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
#include <vtkm/filter/geometry_refinement/Tube.h>
#include <vtkm/filter/geometry_refinement/worklet/Tube.h>

namespace
{
VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& field,
                          const vtkm::worklet::Tube& worklet)
{
  if (field.IsFieldPoint())
  {
    return vtkm::filter::MapFieldPermutation(field, worklet.GetOutputPointSourceIndex(), result);
  }
  else if (field.IsFieldCell())
  {
    return vtkm::filter::MapFieldPermutation(field, worklet.GetOutputCellSourceIndex(), result);
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

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{
VTKM_CONT vtkm::cont::DataSet Tube::DoExecute(const vtkm::cont::DataSet& input)
{
  vtkm::worklet::Tube worklet;

  worklet.SetCapping(this->Capping);
  worklet.SetNumberOfSides(this->NumberOfSides);
  worklet.SetRadius(this->Radius);

  const auto& originalPoints = input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  vtkm::cont::ArrayHandle<vtkm::Vec3f> newPoints;
  vtkm::cont::CellSetSingleType<> newCells;
  worklet.Run(originalPoints.GetDataAsMultiplexer(), input.GetCellSet(), newPoints, newCells);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  // create the output dataset (without a CoordinateSystem).
  vtkm::cont::DataSet output = this->CreateResult(input, newCells, mapper);

  output.AddCoordinateSystem(vtkm::cont::CoordinateSystem(originalPoints.GetName(), newPoints));
  return output;
}
} // namespace geometry_refinement
} // namespace filter
} // namespace vtkm::filter
