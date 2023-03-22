//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/UncertainCellSet.h>
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/geometry_refinement/Shrink.h>
#include <vtkm/filter/geometry_refinement/worklet/Shrink.h>

namespace
{
VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& inputField,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& outputToInputCellMap)
{
  if (inputField.IsCellField() || inputField.IsWholeDataSetField())
  {
    result.AddField(inputField); // pass through
    return true;
  }
  else if (inputField.IsPointField())
  {
    return vtkm::filter::MapFieldPermutation(inputField, outputToInputCellMap, result);
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
VTKM_CONT vtkm::cont::DataSet Shrink::DoExecute(const vtkm::cont::DataSet& input)
{
  const vtkm::cont::UnknownCellSet& inCellSet = input.GetCellSet();
  const auto& oldCoords = input.GetCoordinateSystem().GetDataAsMultiplexer();

  vtkm::cont::ArrayHandle<vtkm::Vec3f> newCoords;
  vtkm::cont::ArrayHandle<vtkm::Id> oldPointsMapping;
  vtkm::cont::CellSetExplicit<> newCellset;
  vtkm::worklet::Shrink worklet;

  worklet.Run(inCellSet, this->ShrinkFactor, oldCoords, newCoords, oldPointsMapping, newCellset);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, oldPointsMapping); };

  vtkm::cont::CoordinateSystem activeCoordSystem = input.GetCoordinateSystem();
  activeCoordSystem = vtkm::cont::CoordinateSystem(activeCoordSystem.GetName(), newCoords);

  return this->CreateResultCoordinateSystem(input, newCellset, activeCoordSystem, mapper);
}
} // namespace geometry_refinement
} // namespace filter
} // namespace vtkm
