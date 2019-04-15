//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_CleanGrid_hxx
#define vtk_m_filter_CleanGrid_hxx

#include <vtkm/filter/CleanGrid.h>

#include <vtkm/worklet/CellDeepCopy.h>
#include <vtkm/worklet/RemoveUnusedPoints.h>

#include <vector>

namespace vtkm
{
namespace filter
{

inline VTKM_CONT CleanGrid::CleanGrid()
  : CompactPointFields(true)
  , MergePoints(true)
  , Tolerance(1.0e-6)
  , ToleranceIsAbsolute(false)
  , RemoveDegenerateCells(true)
  , FastMerge(true)
{
}

template <typename Policy>
inline VTKM_CONT vtkm::cont::DataSet CleanGrid::DoExecute(const vtkm::cont::DataSet& inData,
                                                          vtkm::filter::PolicyBase<Policy> policy)
{
  using CellSetType = vtkm::cont::CellSetExplicit<>;
  using VecId = std::vector<CellSetType>::size_type;

  VecId numCellSets = static_cast<VecId>(inData.GetNumberOfCellSets());
  std::vector<CellSetType> outputCellSets(numCellSets);

  VecId activeCoordIndex = static_cast<VecId>(this->GetActiveCoordinateSystemIndex());

  // Do a deep copy of the cells to new CellSetExplicit structures
  for (VecId cellSetIndex = 0; cellSetIndex < numCellSets; ++cellSetIndex)
  {
    vtkm::cont::DynamicCellSet inCellSet =
      inData.GetCellSet(static_cast<vtkm::IdComponent>(cellSetIndex));
    if (inCellSet.IsType<CellSetType>())
    {
      // Is expected type, do a shallow copy
      outputCellSets[cellSetIndex] = inCellSet.Cast<CellSetType>();
    }
    else
    {
      vtkm::worklet::CellDeepCopy::Run(vtkm::filter::ApplyPolicy(inCellSet, policy),
                                       outputCellSets[cellSetIndex]);
    }
  }

  VecId numCoordSystems = static_cast<VecId>(inData.GetNumberOfCoordinateSystems());
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
    for (VecId cellSetIndex = 0; cellSetIndex < numCellSets; cellSetIndex++)
    {
      this->PointCompactor.FindPoints(outputCellSets[cellSetIndex]);
    }
    this->PointCompactor.FindPointsEnd();

    for (VecId cellSetIndex = 0; cellSetIndex < numCellSets; ++cellSetIndex)
    {
      outputCellSets[cellSetIndex] = this->PointCompactor.MapCellSet(outputCellSets[cellSetIndex]);
    }

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

    for (VecId cellSetIndex = 0; cellSetIndex < numCellSets; ++cellSetIndex)
    {
      outputCellSets[cellSetIndex] = this->PointMerger.MapCellSet(outputCellSets[cellSetIndex]);
    }
  }

  // Optionally remove degenerate cells
  if (this->GetRemoveDegenerateCells())
  {
    outputCellSets[activeCoordIndex] = this->CellCompactor.Run(outputCellSets[activeCoordIndex]);
  }

  // Construct resulting data set with new cell sets
  vtkm::cont::DataSet outData;
  for (VecId cellSetIndex = 0; cellSetIndex < numCellSets; cellSetIndex++)
  {
    outData.AddCellSet(outputCellSets[cellSetIndex]);
  }

  // Pass the coordinate systems
  for (VecId coordSystemIndex = 0; coordSystemIndex < numCoordSystems; ++coordSystemIndex)
  {
    outData.AddCoordinateSystem(outputCoordinateSystems[coordSystemIndex]);
  }

  return outData;
}

template <typename ValueType, typename Storage, typename Policy>
inline VTKM_CONT bool CleanGrid::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<ValueType, Storage>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<Policy>)
{
  if (fieldMeta.IsPointField() && (this->GetCompactPointFields() || this->GetMergePoints()))
  {
    vtkm::cont::ArrayHandle<ValueType> compactedArray;
    if (this->GetCompactPointFields())
    {
      compactedArray = this->PointCompactor.MapPointFieldDeep(input);
      if (this->GetMergePoints())
      {
        compactedArray = this->PointMerger.MapPointField(compactedArray);
      }
    }
    else if (this->GetMergePoints())
    {
      compactedArray = this->PointMerger.MapPointField(input);
    }
    result.AddField(fieldMeta.AsField(compactedArray));
  }
  else if (fieldMeta.IsCellField() && this->GetRemoveDegenerateCells())
  {
    result.AddField(fieldMeta.AsField(this->CellCompactor.ProcessCellField(input)));
  }
  else
  {
    result.AddField(fieldMeta.AsField(input));
  }

  return true;
}
}
}

#endif //vtk_m_filter_CleanGrid_hxx
