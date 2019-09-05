//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

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
  using VecId = std::size_t;

  VecId activeCoordIndex = static_cast<VecId>(this->GetActiveCoordinateSystemIndex());

  CellSetType outputCellSet;
  // Do a deep copy of the cells to new CellSetExplicit structures
  const vtkm::cont::DynamicCellSet& inCellSet = inData.GetCellSet();
  if (inCellSet.IsType<CellSetType>())
  {
    // Is expected type, do a shallow copy
    outputCellSet = inCellSet.Cast<CellSetType>();
  }
  else
  { // Clean the grid
    auto deducedCellSet = vtkm::filter::ApplyPolicy(inCellSet, policy);
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;

    this->Invoke(worklet::CellDeepCopy::CountCellPoints{}, deducedCellSet, numIndices);

    vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
    vtkm::cont::ArrayHandle<vtkm::Id> offsets;
    vtkm::Id connectivitySize;
    vtkm::cont::ConvertNumComponentsToOffsets(numIndices, offsets, connectivitySize);
    numIndices.ReleaseResourcesExecution();

    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    connectivity.Allocate(connectivitySize);

    this->Invoke(worklet::CellDeepCopy::PassCellStructure{},
                 deducedCellSet,
                 shapes,
                 vtkm::cont::make_ArrayHandleGroupVecVariable(connectivity, offsets));
    shapes.ReleaseResourcesExecution();
    offsets.ReleaseResourcesExecution();
    connectivity.ReleaseResourcesExecution();

    outputCellSet.Fill(
      deducedCellSet.GetNumberOfPoints(), shapes, numIndices, connectivity, offsets);

    //Release the input grid from the execution space
    deducedCellSet.ReleaseResourcesExecution();
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
