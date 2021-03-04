
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

#include <vtkm/filter/MapFieldMergeAverage.h>
#include <vtkm/filter/MapFieldPermutation.h>

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

bool CleanGrid::MapFieldOntoOutput(vtkm::cont::DataSet& result, const vtkm::cont::Field& field)
{
  if (field.IsFieldPoint() && (this->GetCompactPointFields() || this->GetMergePoints()))
  {
    vtkm::cont::Field compactedField;
    if (this->GetCompactPointFields())
    {
      bool success = vtkm::filter::MapFieldPermutation(
        field, this->PointCompactor.GetPointScatter().GetOutputToInputMap(), compactedField);
      if (!success)
      {
        return false;
      }
    }
    else
    {
      compactedField = field;
    }
    if (this->GetMergePoints())
    {
      return vtkm::filter::MapFieldMergeAverage(
        compactedField, this->PointMerger.GetMergeKeys(), result);
    }
    else
    {
      result.AddField(compactedField);
      return true;
    }
  }
  else if (field.IsFieldCell() && this->GetRemoveDegenerateCells())
  {
    return vtkm::filter::MapFieldPermutation(field, this->CellCompactor.GetValidCellIds(), result);
  }
  else
  {
    result.AddField(field);
    return true;
  }
}

//-----------------------------------------------------------------------------
template VTKM_FILTER_COMMON_TEMPLATE_EXPORT vtkm::cont::DataSet CleanGrid::DoExecute(
  const vtkm::cont::DataSet& inData,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault> policy);
}
}
