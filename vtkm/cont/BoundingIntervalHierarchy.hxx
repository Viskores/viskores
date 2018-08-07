//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/Bounds.h>
#include <vtkm/Types.h>
#include <vtkm/VecFromPortalPermute.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleReverse.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/BoundingIntervalHierarchy.h>
#include <vtkm/cont/BoundingIntervalHierarchyNode.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorBadDevice.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/internal/DeviceAdapterListHelpers.h>
#include <vtkm/exec/BoundingIntervalHierarchyExec.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace cont
{

using IdArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
using IdPermutationArrayHandle = vtkm::cont::ArrayHandlePermutation<IdArrayHandle, IdArrayHandle>;
using BoundsArrayHandle = vtkm::cont::ArrayHandle<vtkm::Bounds>;
using CoordsArrayHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
using CoordsPermutationArrayHandle =
  vtkm::cont::ArrayHandlePermutation<IdArrayHandle, CoordsArrayHandle>;
using CountingIdArrayHandle = vtkm::cont::ArrayHandleCounting<vtkm::Id>;
using RangeArrayHandle = vtkm::cont::ArrayHandle<vtkm::Range>;
using RangePermutationArrayHandle =
  vtkm::cont::ArrayHandlePermutation<IdArrayHandle, RangeArrayHandle>;
using SplitArrayHandle = vtkm::cont::ArrayHandle<vtkm::worklet::spatialstructure::TreeNode>;
using SplitPermutationArrayHandle =
  vtkm::cont::ArrayHandlePermutation<IdArrayHandle, SplitArrayHandle>;
using SplitPropertiesArrayHandle =
  vtkm::cont::ArrayHandle<vtkm::worklet::spatialstructure::SplitProperties>;
using HandleType = vtkm::cont::VirtualObjectHandle<vtkm::exec::CellLocator>;



template <typename DeviceAdapter>
VTKM_CONT IdArrayHandle
BoundingIntervalHierarchy::CalculateSegmentSizes(const IdArrayHandle& segmentIds, vtkm::Id numCells)
{
  using Algorithms = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
  IdArrayHandle discardKeys;
  IdArrayHandle segmentSizes;
  Algorithms::ReduceByKey(segmentIds,
                          vtkm::cont::ArrayHandleConstant<vtkm::Id>(1, numCells),
                          discardKeys,
                          segmentSizes,
                          vtkm::Add());
  return segmentSizes;
}

template <typename DeviceAdapter>
VTKM_CONT IdArrayHandle
BoundingIntervalHierarchy::GenerateSegmentIds(const IdArrayHandle& segmentSizes, vtkm::Id numCells)
{
  // Compact segment ids, removing non-contiguous values.
  using Algorithms = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
  // 1. Perform ScanInclusive to calculate the end positions of each segment
  IdArrayHandle segmentEnds;
  Algorithms::ScanInclusive(segmentSizes, segmentEnds);
  // 2. Perform UpperBounds to perform the final compaction.
  IdArrayHandle segmentIds;
  Algorithms::UpperBounds(
    segmentEnds, vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, numCells), segmentIds);
  return segmentIds;
}

template <typename DeviceAdapter>
VTKM_CONT void BoundingIntervalHierarchy::CalculateSplitCosts(
  RangePermutationArrayHandle& segmentRanges,
  RangeArrayHandle& ranges,
  CoordsArrayHandle& coords,
  IdArrayHandle& segmentIds,
  SplitPropertiesArrayHandle& splits,
  DeviceAdapter)
{
  for (vtkm::IdComponent planeIndex = 0; planeIndex < NumPlanes; ++planeIndex)
  {
    CalculatePlaneSplitCost(planeIndex,
                            NumPlanes,
                            segmentRanges,
                            ranges,
                            coords,
                            segmentIds,
                            splits,
                            planeIndex,
                            DeviceAdapter());
  }
  // Calculate median costs
  CalculatePlaneSplitCost(
    0, 1, segmentRanges, ranges, coords, segmentIds, splits, NumPlanes, DeviceAdapter());
}

template <typename DeviceAdapter>
VTKM_CONT void BoundingIntervalHierarchy::CalculatePlaneSplitCost(
  vtkm::IdComponent planeIndex,
  vtkm::IdComponent numPlanes,
  RangePermutationArrayHandle& segmentRanges,
  RangeArrayHandle& ranges,
  CoordsArrayHandle& coords,
  IdArrayHandle& segmentIds,
  SplitPropertiesArrayHandle& splits,
  vtkm::IdComponent index,
  DeviceAdapter)
{
  using Algorithms = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
  // Make candidate split plane array
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> splitPlanes;
  vtkm::worklet::spatialstructure::SplitPlaneCalculatorWorklet splitPlaneCalcWorklet(planeIndex,
                                                                                     numPlanes);
  vtkm::worklet::DispatcherMapField<vtkm::worklet::spatialstructure::SplitPlaneCalculatorWorklet,
                                    DeviceAdapter>
    splitDispatcher(splitPlaneCalcWorklet);
  splitDispatcher.Invoke(segmentRanges, splitPlanes);

  // Check if a point is to the left of the split plane or right
  vtkm::cont::ArrayHandle<vtkm::Id> isLEQOfSplitPlane, isROfSplitPlane;
  vtkm::worklet::DispatcherMapField<vtkm::worklet::spatialstructure::LEQWorklet, DeviceAdapter>()
    .Invoke(coords, splitPlanes, isLEQOfSplitPlane, isROfSplitPlane);

  // Count of points to the left
  vtkm::cont::ArrayHandle<vtkm::Id> pointsToLeft;
  IdArrayHandle discardKeys;
  Algorithms::ReduceByKey(segmentIds, isLEQOfSplitPlane, discardKeys, pointsToLeft, vtkm::Add());

  // Count of points to the right
  vtkm::cont::ArrayHandle<vtkm::Id> pointsToRight;
  Algorithms::ReduceByKey(segmentIds, isROfSplitPlane, discardKeys, pointsToRight, vtkm::Add());

  isLEQOfSplitPlane.ReleaseResourcesExecution();
  isROfSplitPlane.ReleaseResourcesExecution();

  // Calculate Lmax and Rmin
  vtkm::cont::ArrayHandle<vtkm::Range> lMaxRanges;
  {
    vtkm::cont::ArrayHandle<vtkm::Range> leqRanges;
    vtkm::worklet::DispatcherMapField<vtkm::worklet::spatialstructure::FilterRanges<true>>().Invoke(
      coords, splitPlanes, ranges, leqRanges);
    Algorithms::ReduceByKey(
      segmentIds, leqRanges, discardKeys, lMaxRanges, vtkm::worklet::spatialstructure::RangeAdd());
  }

  vtkm::cont::ArrayHandle<vtkm::Range> rMinRanges;
  {
    vtkm::cont::ArrayHandle<vtkm::Range> rRanges;
    vtkm::worklet::DispatcherMapField<vtkm::worklet::spatialstructure::FilterRanges<false>>()
      .Invoke(coords, splitPlanes, ranges, rRanges);
    Algorithms::ReduceByKey(
      segmentIds, rRanges, discardKeys, rMinRanges, vtkm::worklet::spatialstructure::RangeAdd());
  }

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> segmentedSplitPlanes;
  Algorithms::ReduceByKey(
    segmentIds, splitPlanes, discardKeys, segmentedSplitPlanes, vtkm::Minimum());

  // Calculate costs
  vtkm::worklet::spatialstructure::SplitPropertiesCalculator splitPropertiesCalculator(
    index, NumPlanes + 1);

  vtkm::worklet::DispatcherMapField<vtkm::worklet::spatialstructure::SplitPropertiesCalculator>(
    splitPropertiesCalculator)
    .Invoke(pointsToLeft, pointsToRight, lMaxRanges, rMinRanges, segmentedSplitPlanes, splits);
}

template <typename DeviceAdapter>
VTKM_CONT IdArrayHandle
BoundingIntervalHierarchy::CalculateSplitScatterIndices(const IdArrayHandle& cellIds,
                                                        const IdArrayHandle& leqFlags,
                                                        const IdArrayHandle& segmentIds,
                                                        DeviceAdapter)
{
  using Algorithms = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
  // Count total number of true flags preceding in segment
  IdArrayHandle trueFlagCounts;
  Algorithms::ScanExclusiveByKey(segmentIds, leqFlags, trueFlagCounts);

  // Make a counting iterator.
  CountingIdArrayHandle counts(0, 1, cellIds.GetNumberOfValues());

  // Total number of elements in previous segment
  vtkm::cont::ArrayHandle<vtkm::Id> countPreviousSegments;
  Algorithms::ScanInclusiveByKey(segmentIds, counts, countPreviousSegments, vtkm::Minimum());

  // Total number of false flags so far in segment
  vtkm::cont::ArrayHandleTransform<IdArrayHandle, vtkm::worklet::spatialstructure::Invert>
    flagsInverse(leqFlags, vtkm::worklet::spatialstructure::Invert());
  vtkm::cont::ArrayHandle<vtkm::Id> runningFalseFlagCount;
  Algorithms::ScanInclusiveByKey(segmentIds, flagsInverse, runningFalseFlagCount, vtkm::Add());

  // Total number of false flags in segment
  IdArrayHandle totalFalseFlagSegmentCount =
    vtkm::worklet::spatialstructure::ReverseScanInclusiveByKey(
      segmentIds, runningFalseFlagCount, vtkm::Maximum(), DeviceAdapter());

  // if point is to the left,
  //    index = total number in  previous segments + total number of false flags in this segment + total number of trues in previous segment
  // else
  //    index = total number in previous segments + number of falses preceding it in the segment.
  IdArrayHandle scatterIndices;
  vtkm::worklet::DispatcherMapField<vtkm::worklet::spatialstructure::SplitIndicesCalculator>()
    .Invoke(leqFlags,
            trueFlagCounts,
            countPreviousSegments,
            runningFalseFlagCount,
            totalFalseFlagSegmentCount,
            scatterIndices);
  return scatterIndices;
}

class BoundingIntervalHierarchy::BuildFunctor
{
protected:
  BoundingIntervalHierarchy* Self;

public:
  VTKM_CONT
  BuildFunctor(BoundingIntervalHierarchy* self)
    : Self(self)
  {
  }

  template <typename DeviceAdapter>
  VTKM_CONT bool operator()(DeviceAdapter)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapter);
    // Accommodate into a Functor, so that this could be used with TryExecute
    using Algorithms = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    vtkm::cont::DynamicCellSet cellSet = Self->GetCellSet();
    vtkm::Id numCells = cellSet.GetNumberOfCells();
    vtkm::cont::CoordinateSystem coords = Self->GetCoordinates();
    vtkm::cont::ArrayHandleVirtualCoordinates points = coords.GetData();

    //std::cout << "No of cells: " << numCells << "\n";
    //std::cout.precision(3);
    //START_TIMER(s11);
    IdArrayHandle cellIds;
    Algorithms::Copy(CountingIdArrayHandle(0, 1, numCells), cellIds);
    IdArrayHandle segmentIds;
    Algorithms::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, numCells), segmentIds);
    //PRINT_TIMER("1.1", s11);

    //START_TIMER(s12);
    CoordsArrayHandle centerXs, centerYs, centerZs;
    RangeArrayHandle xRanges, yRanges, zRanges;
    vtkm::worklet::DispatcherMapTopology<vtkm::worklet::spatialstructure::CellRangesExtracter,
                                         DeviceAdapter>()
      .Invoke(cellSet, points, xRanges, yRanges, zRanges, centerXs, centerYs, centerZs);
    //PRINT_TIMER("1.2", s12);

    bool done = false;
    //vtkm::IdComponent iteration = 0;
    vtkm::Id nodesIndexOffset = 0;
    vtkm::Id numSegments = 1;
    IdArrayHandle discardKeys;
    IdArrayHandle segmentStarts;
    IdArrayHandle segmentSizes;
    segmentSizes.Allocate(1);
    segmentSizes.GetPortalControl().Set(0, numCells);
    Self->ProcessedCellIds.Allocate(numCells);
    vtkm::Id cellIdsOffset = 0;

    while (!done)
    {
      //std::cout << "**** Iteration " << (++iteration) << " ****\n";
      //Output(segmentSizes);
      //START_TIMER(s21);
      // Calculate the X, Y, Z bounding ranges for each segment
      RangeArrayHandle perSegmentXRanges, perSegmentYRanges, perSegmentZRanges;
      Algorithms::ReduceByKey(segmentIds, xRanges, discardKeys, perSegmentXRanges, vtkm::Add());
      Algorithms::ReduceByKey(segmentIds, yRanges, discardKeys, perSegmentYRanges, vtkm::Add());
      Algorithms::ReduceByKey(segmentIds, zRanges, discardKeys, perSegmentZRanges, vtkm::Add());
      //PRINT_TIMER("2.1", s21);

      // Expand the per segment bounding ranges, to per cell;
      RangePermutationArrayHandle segmentXRanges(segmentIds, perSegmentXRanges);
      RangePermutationArrayHandle segmentYRanges(segmentIds, perSegmentYRanges);
      RangePermutationArrayHandle segmentZRanges(segmentIds, perSegmentZRanges);

      //START_TIMER(s22);
      // Calculate split costs for NumPlanes split planes, across X, Y and Z dimensions
      vtkm::Id numSplitPlanes = numSegments * (Self->NumPlanes + 1);
      vtkm::cont::ArrayHandle<vtkm::worklet::spatialstructure::SplitProperties> xSplits, ySplits,
        zSplits;
      xSplits.Allocate(numSplitPlanes);
      ySplits.Allocate(numSplitPlanes);
      zSplits.Allocate(numSplitPlanes);
      Self->CalculateSplitCosts(
        segmentXRanges, xRanges, centerXs, segmentIds, xSplits, DeviceAdapter());
      Self->CalculateSplitCosts(
        segmentYRanges, yRanges, centerYs, segmentIds, ySplits, DeviceAdapter());
      Self->CalculateSplitCosts(
        segmentZRanges, zRanges, centerZs, segmentIds, zSplits, DeviceAdapter());
      //PRINT_TIMER("2.2", s22);

      segmentXRanges.ReleaseResourcesExecution();
      segmentYRanges.ReleaseResourcesExecution();
      segmentZRanges.ReleaseResourcesExecution();

      //START_TIMER(s23);
      // Select best split plane and dimension across X, Y, Z dimension, per segment
      SplitArrayHandle segmentSplits;
      vtkm::cont::ArrayHandle<vtkm::FloatDefault> segmentPlanes;
      vtkm::cont::ArrayHandle<vtkm::Id> splitChoices;
      vtkm::worklet::spatialstructure::SplitSelector worklet(
        Self->NumPlanes, Self->MaxLeafSize, Self->NumPlanes + 1);
      vtkm::worklet::DispatcherMapField<vtkm::worklet::spatialstructure::SplitSelector>
        splitSelectorDispatcher(worklet);
      CountingIdArrayHandle indices(0, 1, numSegments);
      splitSelectorDispatcher.Invoke(indices,
                                     xSplits,
                                     ySplits,
                                     zSplits,
                                     segmentSizes,
                                     segmentSplits,
                                     segmentPlanes,
                                     splitChoices);
      //PRINT_TIMER("2.3", s23);

      // Expand the per segment split plane to per cell
      SplitPermutationArrayHandle splits(segmentIds, segmentSplits);
      CoordsPermutationArrayHandle planes(segmentIds, segmentPlanes);

      //START_TIMER(s31);
      IdArrayHandle leqFlags;
      vtkm::worklet::DispatcherMapField<
        vtkm::worklet::spatialstructure::CalculateSplitDirectionFlag>
        computeFlagDispatcher;
      computeFlagDispatcher.Invoke(centerXs, centerYs, centerZs, splits, planes, leqFlags);
      //PRINT_TIMER("3.1", s31);

      //START_TIMER(s32);
      IdArrayHandle scatterIndices =
        Self->CalculateSplitScatterIndices(cellIds, leqFlags, segmentIds, DeviceAdapter());
      IdArrayHandle newSegmentIds;
      IdPermutationArrayHandle sizes(segmentIds, segmentSizes);
      vtkm::worklet::DispatcherMapField<vtkm::worklet::spatialstructure::SegmentSplitter>(
        vtkm::worklet::spatialstructure::SegmentSplitter(Self->MaxLeafSize))
        .Invoke(segmentIds, leqFlags, sizes, newSegmentIds);
      //PRINT_TIMER("3.2", s32);

      //START_TIMER(s33);
      vtkm::cont::ArrayHandle<vtkm::Id> choices;
      Algorithms::Copy(IdPermutationArrayHandle(segmentIds, splitChoices), choices);
      cellIds = vtkm::worklet::spatialstructure::ScatterArray(cellIds, scatterIndices);
      segmentIds = vtkm::worklet::spatialstructure::ScatterArray(segmentIds, scatterIndices);
      newSegmentIds = vtkm::worklet::spatialstructure::ScatterArray(newSegmentIds, scatterIndices);
      xRanges = vtkm::worklet::spatialstructure::ScatterArray(xRanges, scatterIndices);
      yRanges = vtkm::worklet::spatialstructure::ScatterArray(yRanges, scatterIndices);
      zRanges = vtkm::worklet::spatialstructure::ScatterArray(zRanges, scatterIndices);
      centerXs = vtkm::worklet::spatialstructure::ScatterArray(centerXs, scatterIndices);
      centerYs = vtkm::worklet::spatialstructure::ScatterArray(centerYs, scatterIndices);
      centerZs = vtkm::worklet::spatialstructure::ScatterArray(centerZs, scatterIndices);
      choices = vtkm::worklet::spatialstructure::ScatterArray(choices, scatterIndices);
      //PRINT_TIMER("3.3", s33);

      // Move the cell ids at leafs to the processed cellids list
      //START_TIMER(s41);
      IdArrayHandle nonSplitSegmentSizes;
      vtkm::worklet::DispatcherMapField<vtkm::worklet::spatialstructure::NonSplitIndexCalculator>(
        vtkm::worklet::spatialstructure::NonSplitIndexCalculator(Self->MaxLeafSize))
        .Invoke(segmentSizes, nonSplitSegmentSizes);
      IdArrayHandle nonSplitSegmentIndices;
      Algorithms::ScanExclusive(nonSplitSegmentSizes, nonSplitSegmentIndices);
      IdArrayHandle runningSplitSegmentCounts;
      Algorithms::ScanExclusive(splitChoices, runningSplitSegmentCounts);
      //PRINT_TIMER("4.1", s41);

      //START_TIMER(s42);
      IdArrayHandle doneCellIds;
      Algorithms::CopyIf(cellIds, choices, doneCellIds, vtkm::worklet::spatialstructure::Invert());
      Algorithms::CopySubRange(
        doneCellIds, 0, doneCellIds.GetNumberOfValues(), Self->ProcessedCellIds, cellIdsOffset);

      cellIds = vtkm::worklet::spatialstructure::CopyIfArray(cellIds, choices, DeviceAdapter());
      newSegmentIds =
        vtkm::worklet::spatialstructure::CopyIfArray(newSegmentIds, choices, DeviceAdapter());
      xRanges = vtkm::worklet::spatialstructure::CopyIfArray(xRanges, choices, DeviceAdapter());
      yRanges = vtkm::worklet::spatialstructure::CopyIfArray(yRanges, choices, DeviceAdapter());
      zRanges = vtkm::worklet::spatialstructure::CopyIfArray(zRanges, choices, DeviceAdapter());
      centerXs = vtkm::worklet::spatialstructure::CopyIfArray(centerXs, choices, DeviceAdapter());
      centerYs = vtkm::worklet::spatialstructure::CopyIfArray(centerYs, choices, DeviceAdapter());
      centerZs = vtkm::worklet::spatialstructure::CopyIfArray(centerZs, choices, DeviceAdapter());
      //PRINT_TIMER("4.2", s42);

      //START_TIMER(s43);
      // Make a new nodes with enough nodes for the current level, copying over the old one
      vtkm::Id nodesSize = Self->Nodes.GetNumberOfValues() + numSegments;
      vtkm::cont::ArrayHandle<BoundingIntervalHierarchyNode> newTree;
      newTree.Allocate(nodesSize);
      Algorithms::CopySubRange(Self->Nodes, 0, Self->Nodes.GetNumberOfValues(), newTree);

      CountingIdArrayHandle nodesIndices(nodesIndexOffset, 1, numSegments);
      vtkm::worklet::spatialstructure::TreeLevelAdder nodesAdder(
        cellIdsOffset, nodesSize, Self->MaxLeafSize);
      vtkm::worklet::DispatcherMapField<vtkm::worklet::spatialstructure::TreeLevelAdder>(nodesAdder)
        .Invoke(nodesIndices,
                segmentSplits,
                nonSplitSegmentIndices,
                segmentSizes,
                runningSplitSegmentCounts,
                newTree);
      nodesIndexOffset = nodesSize;
      cellIdsOffset += doneCellIds.GetNumberOfValues();
      Self->Nodes = newTree;
      //PRINT_TIMER("4.3", s43);
      //START_TIMER(s51);
      segmentIds = newSegmentIds;
      segmentSizes =
        Self->CalculateSegmentSizes<DeviceAdapter>(segmentIds, segmentIds.GetNumberOfValues());
      segmentIds =
        Self->GenerateSegmentIds<DeviceAdapter>(segmentSizes, segmentIds.GetNumberOfValues());
      IdArrayHandle uniqueSegmentIds;
      Algorithms::Copy(segmentIds, uniqueSegmentIds);
      Algorithms::Unique(uniqueSegmentIds);
      numSegments = uniqueSegmentIds.GetNumberOfValues();
      done = segmentIds.GetNumberOfValues() == 0;
      //PRINT_TIMER("5.1", s51);
      //std::cout << "Iteration time: " << iterationTimer.GetElapsedTime() << "\n";
    }
    //std::cout << "Total time: " << totalTimer.GetElapsedTime() << "\n";
    return true;
  }
};

class BoundingIntervalHierarchy::PrepareForExecutionFunctor
{
public:
  template <typename DeviceAdapter>
  VTKM_CONT void operator()(DeviceAdapter,
                            const vtkm::cont::BoundingIntervalHierarchy& bih,
                            HandleType& bihExec) const
  {
    vtkm::cont::DynamicCellSet cellSet = bih.GetCellSet();
    if (cellSet.IsType<vtkm::cont::CellSetExplicit<>>())
    {
      using CellSetType = vtkm::cont::CellSetExplicit<>;
      using ExecutionType = vtkm::exec::BoundingIntervalHierarchyExec<DeviceAdapter, CellSetType>;
      ExecutionType* execObject = new ExecutionType(bih.Nodes,
                                                    bih.ProcessedCellIds,
                                                    bih.GetCellSet().Cast<CellSetType>(),
                                                    bih.GetCoordinates().GetData(),
                                                    DeviceAdapter());
      bihExec.Reset(execObject);
    }
    else if (cellSet.IsType<vtkm::cont::CellSetStructured<2>>())
    {
      using CellSetType = vtkm::cont::CellSetStructured<2>;
      using ExecutionType = vtkm::exec::BoundingIntervalHierarchyExec<DeviceAdapter, CellSetType>;
      ExecutionType* execObject = new ExecutionType(bih.Nodes,
                                                    bih.ProcessedCellIds,
                                                    bih.GetCellSet().Cast<CellSetType>(),
                                                    bih.GetCoordinates().GetData(),
                                                    DeviceAdapter());
      bihExec.Reset(execObject);
    }
    else if (cellSet.IsType<vtkm::cont::CellSetStructured<3>>())
    {
      using CellSetType = vtkm::cont::CellSetStructured<3>;
      using ExecutionType = vtkm::exec::BoundingIntervalHierarchyExec<DeviceAdapter, CellSetType>;
      ExecutionType* execObject = new ExecutionType(bih.Nodes,
                                                    bih.ProcessedCellIds,
                                                    bih.GetCellSet().Cast<CellSetType>(),
                                                    bih.GetCoordinates().GetData(),
                                                    DeviceAdapter());
      bihExec.Reset(execObject);
    }
    else if (cellSet.IsType<vtkm::cont::CellSetSingleType<>>())
    {
      using CellSetType = vtkm::cont::CellSetSingleType<>;
      using ExecutionType = vtkm::exec::BoundingIntervalHierarchyExec<DeviceAdapter, CellSetType>;
      ExecutionType* execObject = new ExecutionType(bih.Nodes,
                                                    bih.ProcessedCellIds,
                                                    bih.GetCellSet().Cast<CellSetType>(),
                                                    bih.GetCoordinates().GetData(),
                                                    DeviceAdapter());
      bihExec.Reset(execObject);
    }
    else
    {
      throw vtkm::cont::ErrorBadType("Could not determine type to write out.");
    }
  }
};

VTKM_CONT
void BoundingIntervalHierarchy::Build()
{
  BuildFunctor functor(this);
  vtkm::cont::TryExecute(functor);
}

VTKM_CONT
const HandleType BoundingIntervalHierarchy::PrepareForExecutionImpl(
  const vtkm::cont::DeviceAdapterId deviceId) const
{
  using DeviceList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG;
  vtkm::cont::internal::FindDeviceAdapterTagAndCall(
    deviceId, DeviceList(), PrepareForExecutionFunctor(), *this, this->ExecHandle);

  return this->ExecHandle;
}

} //namespace cont
} //namespace vtkm
