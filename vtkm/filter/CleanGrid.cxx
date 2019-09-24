
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define vtkm_filter_CleanGrid_cxx
#include <vtkm/filter/CleanGrid.h>
#include <vtkm/filter/CleanGrid.hxx>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
CleanGrid::CleanGrid()
  : CompactPointFields(true)
  , MergePoints(true)
  , Tolerance(1.0e-6)
  , ToleranceIsAbsolute(false)
  , RemoveDegenerateCells(true)
  , FastMerge(true)
{
}

//-----------------------------------------------------------------------------
vtkm::cont::DataSet CleanGrid::GenerateOutput(const vtkm::cont::DataSet& inData,
                                              vtkm::cont::CellSetExplicit<>& outputCellSet)
{
  using VecId = std::size_t;
  const VecId activeCoordIndex = static_cast<VecId>(this->GetActiveCoordinateSystemIndex());
  const VecId numCoordSystems = static_cast<VecId>(inData.GetNumberOfCoordinateSystems());

  std::vector<vtkm::cont::CoordinateSystem> outputCoordinateSystems(numCoordSystems);

  // Start with a shallow copy of the coordinate systems
  for (VecId coordSystemIndex = 0; coordSystemIndex < numCoordSystems; ++coordSystemIndex)
  {
    outputCoordinateSystems[coordSystemIndex] =
      inData.GetCoordinateSystem(static_cast<vtkm::IdComponent>(coordSystemIndex));
  }

  // Optionally adjust the cell set indices to remove all unused points
  if (this->GetCompactPointFields())
  {
    this->PointCompactor.FindPointsStart();
    this->PointCompactor.FindPoints(outputCellSet);
    this->PointCompactor.FindPointsEnd();

    outputCellSet = this->PointCompactor.MapCellSet(outputCellSet);

    for (VecId coordSystemIndex = 0; coordSystemIndex < numCoordSystems; ++coordSystemIndex)
    {
      outputCoordinateSystems[coordSystemIndex] =
        vtkm::cont::CoordinateSystem(outputCoordinateSystems[coordSystemIndex].GetName(),
                                     this->PointCompactor.MapPointFieldDeep(
                                       outputCoordinateSystems[coordSystemIndex].GetData()));
    }
  }

  // Optionally find and merge coincident points
  if (this->GetMergePoints())
  {
    vtkm::cont::CoordinateSystem activeCoordSystem = outputCoordinateSystems[activeCoordIndex];
    vtkm::Bounds bounds = activeCoordSystem.GetBounds();

    vtkm::Float64 delta = this->GetTolerance();
    if (!this->GetToleranceIsAbsolute())
    {
      delta *=
        vtkm::Magnitude(vtkm::make_Vec(bounds.X.Length(), bounds.Y.Length(), bounds.Z.Length()));
    }

    auto coordArray = activeCoordSystem.GetData();
    this->PointMerger.Run(delta, this->GetFastMerge(), bounds, coordArray);
    activeCoordSystem = vtkm::cont::CoordinateSystem(activeCoordSystem.GetName(), coordArray);

    for (VecId coordSystemIndex = 0; coordSystemIndex < numCoordSystems; ++coordSystemIndex)
    {
      if (coordSystemIndex == activeCoordIndex)
      {
        outputCoordinateSystems[coordSystemIndex] = activeCoordSystem;
      }
      else
      {
        outputCoordinateSystems[coordSystemIndex] = vtkm::cont::CoordinateSystem(
          outputCoordinateSystems[coordSystemIndex].GetName(),
          this->PointMerger.MapPointField(outputCoordinateSystems[coordSystemIndex].GetData()));
      }
    }

    outputCellSet = this->PointMerger.MapCellSet(outputCellSet);
  }

  // Optionally remove degenerate cells
  if (this->GetRemoveDegenerateCells())
  {
    outputCellSet = this->CellCompactor.Run(outputCellSet);
  }

  // Construct resulting data set with new cell sets
  vtkm::cont::DataSet outData;
  outData.SetCellSet(outputCellSet);

  // Pass the coordinate systems
  for (VecId coordSystemIndex = 0; coordSystemIndex < numCoordSystems; ++coordSystemIndex)
  {
    outData.AddCoordinateSystem(outputCoordinateSystems[coordSystemIndex]);
  }

  return outData;
}

//-----------------------------------------------------------------------------
VTKM_FILTER_INSTANTIATE_EXECUTE_METHOD(CleanGrid);
}
}
