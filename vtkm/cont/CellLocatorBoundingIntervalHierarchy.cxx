//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/CellLocatorBoundingIntervalHierarchy.h>

#include <vtkm/Bounds.h>
#include <vtkm/Types.h>
#include <vtkm/VecFromPortalPermute.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleReverse.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorBadDevice.h>
#include <vtkm/exec/CellLocatorBoundingIntervalHierarchyExec.h>

#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace cont
{

using IdArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
using IdPermutationArrayHandle = vtkm::cont::ArrayHandlePermutation<IdArrayHandle, IdArrayHandle>;
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

namespace
{

IdArrayHandle CalculateSegmentSizes(const IdArrayHandle& segmentIds, vtkm::Id numCells)
{
  IdArrayHandle discardKeys;
  IdArrayHandle segmentSizes;
  vtkm::cont::Algorithm::ReduceByKey(segmentIds,
                                     vtkm::cont::ArrayHandleConstant<vtkm::Id>(1, numCells),
                                     discardKeys,
                                     segmentSizes,
                                     vtkm::Add());
  return segmentSizes;
}

IdArrayHandle GenerateSegmentIds(const IdArrayHandle& segmentSizes, vtkm::Id numCells)
{
  // Compact segment ids, removing non-contiguous values.

  // 1. Perform ScanInclusive to calculate the end positions of each segment
  IdArrayHandle segmentEnds;
  vtkm::cont::Algorithm::ScanInclusive(segmentSizes, segmentEnds);
  // 2. Perform UpperBounds to perform the final compaction.
  IdArrayHandle segmentIds;
  vtkm::cont::Algorithm::UpperBounds(
    segmentEnds, vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, numCells), segmentIds);
  return segmentIds;
}

void CalculatePlaneSplitCost(vtkm::IdComponent planeIndex,
                             vtkm::IdComponent numPlanes,
                             RangePermutationArrayHandle& segmentRanges,
                             RangeArrayHandle& ranges,
                             CoordsArrayHandle& coords,
                             IdArrayHandle& segmentIds,
                             SplitPropertiesArrayHandle& splits,
                             vtkm::IdComponent index,
                             vtkm::IdComponent numTotalPlanes)
{
  vtkm::cont::Invoker invoker;

  // Make candidate split plane array
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> splitPlanes;
  vtkm::worklet::spatialstructure::SplitPlaneCalculatorWorklet splitPlaneCalcWorklet(planeIndex,
                                                                                     numPlanes);
  invoker(splitPlaneCalcWorklet, segmentRanges, splitPlanes);

  // Check if a point is to the left of the split plane or right
  vtkm::cont::ArrayHandle<vtkm::Id> isLEQOfSplitPlane, isROfSplitPlane;
  invoker(vtkm::worklet::spatialstructure::LEQWorklet{},
          coords,
          splitPlanes,
          isLEQOfSplitPlane,
          isROfSplitPlane);

  // Count of points to the left
  vtkm::cont::ArrayHandle<vtkm::Id> pointsToLeft;
  IdArrayHandle discardKeys;
  vtkm::cont::Algorithm::ReduceByKey(
    segmentIds, isLEQOfSplitPlane, discardKeys, pointsToLeft, vtkm::Add());

  // Count of points to the right
  vtkm::cont::ArrayHandle<vtkm::Id> pointsToRight;
  vtkm::cont::Algorithm::ReduceByKey(
    segmentIds, isROfSplitPlane, discardKeys, pointsToRight, vtkm::Add());

  isLEQOfSplitPlane.ReleaseResourcesExecution();
  isROfSplitPlane.ReleaseResourcesExecution();

  // Calculate Lmax and Rmin
  vtkm::cont::ArrayHandle<vtkm::Range> lMaxRanges;
  {
    vtkm::cont::ArrayHandle<vtkm::Range> leqRanges;
    vtkm::worklet::spatialstructure::FilterRanges<true> worklet;
    invoker(worklet, coords, splitPlanes, ranges, leqRanges);

    vtkm::cont::Algorithm::ReduceByKey(
      segmentIds, leqRanges, discardKeys, lMaxRanges, vtkm::worklet::spatialstructure::RangeAdd());
  }

  vtkm::cont::ArrayHandle<vtkm::Range> rMinRanges;
  {
    vtkm::cont::ArrayHandle<vtkm::Range> rRanges;
    vtkm::worklet::spatialstructure::FilterRanges<false> worklet;
    invoker(worklet, coords, splitPlanes, ranges, rRanges);

    vtkm::cont::Algorithm::ReduceByKey(
      segmentIds, rRanges, discardKeys, rMinRanges, vtkm::worklet::spatialstructure::RangeAdd());
  }

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> segmentedSplitPlanes;
  vtkm::cont::Algorithm::ReduceByKey(
    segmentIds, splitPlanes, discardKeys, segmentedSplitPlanes, vtkm::Minimum());

  // Calculate costs
  vtkm::worklet::spatialstructure::SplitPropertiesCalculator splitPropertiesCalculator(
    index, numTotalPlanes + 1);
  invoker(splitPropertiesCalculator,
          pointsToLeft,
          pointsToRight,
          lMaxRanges,
          rMinRanges,
          segmentedSplitPlanes,
          splits);
}

void CalculateSplitCosts(vtkm::IdComponent numPlanes,
                         RangePermutationArrayHandle& segmentRanges,
                         RangeArrayHandle& ranges,
                         CoordsArrayHandle& coords,
                         IdArrayHandle& segmentIds,
                         SplitPropertiesArrayHandle& splits)
{
  for (vtkm::IdComponent planeIndex = 0; planeIndex < numPlanes; ++planeIndex)
  {
    CalculatePlaneSplitCost(planeIndex,
                            numPlanes,
                            segmentRanges,
                            ranges,
                            coords,
                            segmentIds,
                            splits,
                            planeIndex,
                            numPlanes);
  }
  // Calculate median costs
  CalculatePlaneSplitCost(
    0, 1, segmentRanges, ranges, coords, segmentIds, splits, numPlanes, numPlanes);
}

IdArrayHandle CalculateSplitScatterIndices(const IdArrayHandle& cellIds,
                                           const IdArrayHandle& leqFlags,
                                           const IdArrayHandle& segmentIds)
{
  vtkm::cont::Invoker invoker;

  // Count total number of true flags preceding in segment
  IdArrayHandle trueFlagCounts;
  vtkm::cont::Algorithm::ScanExclusiveByKey(segmentIds, leqFlags, trueFlagCounts);

  // Make a counting iterator.
  CountingIdArrayHandle counts(0, 1, cellIds.GetNumberOfValues());

  // Total number of elements in previous segment
  vtkm::cont::ArrayHandle<vtkm::Id> countPreviousSegments;
  vtkm::cont::Algorithm::ScanInclusiveByKey(
    segmentIds, counts, countPreviousSegments, vtkm::Minimum());

  // Total number of false flags so far in segment
  vtkm::cont::ArrayHandleTransform<IdArrayHandle, vtkm::worklet::spatialstructure::Invert>
    flagsInverse(leqFlags, vtkm::worklet::spatialstructure::Invert());
  vtkm::cont::ArrayHandle<vtkm::Id> runningFalseFlagCount;
  vtkm::cont::Algorithm::ScanInclusiveByKey(
    segmentIds, flagsInverse, runningFalseFlagCount, vtkm::Add());

  // Total number of false flags in segment
  IdArrayHandle totalFalseFlagSegmentCount =
    vtkm::worklet::spatialstructure::ReverseScanInclusiveByKey(
      segmentIds, runningFalseFlagCount, vtkm::Maximum());

  // if point is to the left,
  //    index = total number in  previous segments + total number of false flags in this segment + total number of trues in previous segment
  // else
  //    index = total number in previous segments + number of falses preceding it in the segment.
  IdArrayHandle scatterIndices;
  invoker(vtkm::worklet::spatialstructure::SplitIndicesCalculator{},
          leqFlags,
          trueFlagCounts,
          countPreviousSegments,
          runningFalseFlagCount,
          totalFalseFlagSegmentCount,
          scatterIndices);
  return scatterIndices;
}

} // anonymous namespace

CellLocatorBoundingIntervalHierarchy::~CellLocatorBoundingIntervalHierarchy() = default;


void CellLocatorBoundingIntervalHierarchy::Build()
{
  vtkm::cont::Invoker invoker;

  vtkm::cont::DynamicCellSet cellSet = this->GetCellSet();
  vtkm::Id numCells = cellSet.GetNumberOfCells();
  vtkm::cont::CoordinateSystem coords = this->GetCoordinates();
  vtkm::cont::ArrayHandleVirtualCoordinates points = coords.GetData();

  //std::cout << "No of cells: " << numCells << "\n";
  //std::cout.precision(3);
  //START_TIMER(s11);
  IdArrayHandle cellIds;
  vtkm::cont::Algorithm::Copy(CountingIdArrayHandle(0, 1, numCells), cellIds);
  IdArrayHandle segmentIds;
  vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, numCells), segmentIds);
  //PRINT_TIMER("1.1", s11);

  //START_TIMER(s12);
  CoordsArrayHandle centerXs, centerYs, centerZs;
  RangeArrayHandle xRanges, yRanges, zRanges;
  invoker(vtkm::worklet::spatialstructure::CellRangesExtracter{},
          cellSet,
          points,
          xRanges,
          yRanges,
          zRanges,
          centerXs,
          centerYs,
          centerZs);
  //PRINT_TIMER("1.2", s12);

  bool done = false;
  //vtkm::IdComponent iteration = 0;
  vtkm::Id nodesIndexOffset = 0;
  vtkm::Id numSegments = 1;
  IdArrayHandle discardKeys;
  IdArrayHandle segmentSizes;
  segmentSizes.Allocate(1);
  segmentSizes.GetPortalControl().Set(0, numCells);
  this->ProcessedCellIds.Allocate(numCells);
  vtkm::Id cellIdsOffset = 0;

  IdArrayHandle parentIndices;
  parentIndices.Allocate(1);
  parentIndices.GetPortalControl().Set(0, -1);

  while (!done)
  {
    //std::cout << "**** Iteration " << (++iteration) << " ****\n";
    //Output(segmentSizes);
    //START_TIMER(s21);
    // Calculate the X, Y, Z bounding ranges for each segment
    RangeArrayHandle perSegmentXRanges, perSegmentYRanges, perSegmentZRanges;
    vtkm::cont::Algorithm::ReduceByKey(
      segmentIds, xRanges, discardKeys, perSegmentXRanges, vtkm::Add());
    vtkm::cont::Algorithm::ReduceByKey(
      segmentIds, yRanges, discardKeys, perSegmentYRanges, vtkm::Add());
    vtkm::cont::Algorithm::ReduceByKey(
      segmentIds, zRanges, discardKeys, perSegmentZRanges, vtkm::Add());
    //PRINT_TIMER("2.1", s21);

    // Expand the per segment bounding ranges, to per cell;
    RangePermutationArrayHandle segmentXRanges(segmentIds, perSegmentXRanges);
    RangePermutationArrayHandle segmentYRanges(segmentIds, perSegmentYRanges);
    RangePermutationArrayHandle segmentZRanges(segmentIds, perSegmentZRanges);

    //START_TIMER(s22);
    // Calculate split costs for NumPlanes split planes, across X, Y and Z dimensions
    vtkm::Id numSplitPlanes = numSegments * (this->NumPlanes + 1);
    vtkm::cont::ArrayHandle<vtkm::worklet::spatialstructure::SplitProperties> xSplits, ySplits,
      zSplits;
    xSplits.Allocate(numSplitPlanes);
    ySplits.Allocate(numSplitPlanes);
    zSplits.Allocate(numSplitPlanes);
    CalculateSplitCosts(this->NumPlanes, segmentXRanges, xRanges, centerXs, segmentIds, xSplits);
    CalculateSplitCosts(this->NumPlanes, segmentYRanges, yRanges, centerYs, segmentIds, ySplits);
    CalculateSplitCosts(this->NumPlanes, segmentZRanges, zRanges, centerZs, segmentIds, zSplits);
    //PRINT_TIMER("2.2", s22);

    segmentXRanges.ReleaseResourcesExecution();
    segmentYRanges.ReleaseResourcesExecution();
    segmentZRanges.ReleaseResourcesExecution();

    //START_TIMER(s23);
    // Select best split plane and dimension across X, Y, Z dimension, per segment
    SplitArrayHandle segmentSplits;
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> segmentPlanes;
    vtkm::cont::ArrayHandle<vtkm::Id> splitChoices;
    CountingIdArrayHandle indices(0, 1, numSegments);

    vtkm::worklet::spatialstructure::SplitSelector worklet(
      this->NumPlanes, this->MaxLeafSize, this->NumPlanes + 1);
    invoker(worklet,
            indices,
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
    invoker(vtkm::worklet::spatialstructure::CalculateSplitDirectionFlag{},
            centerXs,
            centerYs,
            centerZs,
            splits,
            planes,
            leqFlags);
    //PRINT_TIMER("3.1", s31);

    //START_TIMER(s32);
    IdArrayHandle scatterIndices = CalculateSplitScatterIndices(cellIds, leqFlags, segmentIds);
    IdArrayHandle newSegmentIds;
    IdPermutationArrayHandle sizes(segmentIds, segmentSizes);
    invoker(vtkm::worklet::spatialstructure::SegmentSplitter{ this->MaxLeafSize },
            segmentIds,
            leqFlags,
            sizes,
            newSegmentIds);
    //PRINT_TIMER("3.2", s32);

    //START_TIMER(s33);
    vtkm::cont::ArrayHandle<vtkm::Id> choices;
    vtkm::cont::Algorithm::Copy(IdPermutationArrayHandle(segmentIds, splitChoices), choices);
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
    invoker(vtkm::worklet::spatialstructure::NonSplitIndexCalculator{ this->MaxLeafSize },
            segmentSizes,
            nonSplitSegmentSizes);
    IdArrayHandle nonSplitSegmentIndices;
    vtkm::cont::Algorithm::ScanExclusive(nonSplitSegmentSizes, nonSplitSegmentIndices);
    IdArrayHandle runningSplitSegmentCounts;
    vtkm::Id numNewSegments =
      vtkm::cont::Algorithm::ScanExclusive(splitChoices, runningSplitSegmentCounts);
    //PRINT_TIMER("4.1", s41);

    //START_TIMER(s42);
    IdArrayHandle doneCellIds;
    vtkm::cont::Algorithm::CopyIf(
      cellIds, choices, doneCellIds, vtkm::worklet::spatialstructure::Invert());
    vtkm::cont::Algorithm::CopySubRange(
      doneCellIds, 0, doneCellIds.GetNumberOfValues(), this->ProcessedCellIds, cellIdsOffset);

    cellIds = vtkm::worklet::spatialstructure::CopyIfArray(cellIds, choices);
    newSegmentIds = vtkm::worklet::spatialstructure::CopyIfArray(newSegmentIds, choices);
    xRanges = vtkm::worklet::spatialstructure::CopyIfArray(xRanges, choices);
    yRanges = vtkm::worklet::spatialstructure::CopyIfArray(yRanges, choices);
    zRanges = vtkm::worklet::spatialstructure::CopyIfArray(zRanges, choices);
    centerXs = vtkm::worklet::spatialstructure::CopyIfArray(centerXs, choices);
    centerYs = vtkm::worklet::spatialstructure::CopyIfArray(centerYs, choices);
    centerZs = vtkm::worklet::spatialstructure::CopyIfArray(centerZs, choices);
    //PRINT_TIMER("4.2", s42);

    //START_TIMER(s43);
    // Make a new nodes with enough nodes for the current level, copying over the old one
    vtkm::Id nodesSize = this->Nodes.GetNumberOfValues() + numSegments;
    vtkm::cont::ArrayHandle<vtkm::exec::CellLocatorBoundingIntervalHierarchyNode> newTree;
    newTree.Allocate(nodesSize);
    vtkm::cont::Algorithm::CopySubRange(this->Nodes, 0, this->Nodes.GetNumberOfValues(), newTree);

    IdArrayHandle nextParentIndices;
    nextParentIndices.Allocate(2 * numNewSegments);

    CountingIdArrayHandle nodesIndices(nodesIndexOffset, 1, numSegments);
    vtkm::worklet::spatialstructure::TreeLevelAdder nodesAdder(
      cellIdsOffset, nodesSize, this->MaxLeafSize);
    invoker(nodesAdder,
            nodesIndices,
            segmentSplits,
            nonSplitSegmentIndices,
            segmentSizes,
            runningSplitSegmentCounts,
            parentIndices,
            newTree,
            nextParentIndices);
    nodesIndexOffset = nodesSize;
    cellIdsOffset += doneCellIds.GetNumberOfValues();
    this->Nodes = newTree;
    //PRINT_TIMER("4.3", s43);
    //START_TIMER(s51);
    segmentIds = newSegmentIds;
    segmentSizes = CalculateSegmentSizes(segmentIds, segmentIds.GetNumberOfValues());
    segmentIds = GenerateSegmentIds(segmentSizes, segmentIds.GetNumberOfValues());
    IdArrayHandle uniqueSegmentIds;
    vtkm::cont::Algorithm::Copy(segmentIds, uniqueSegmentIds);
    vtkm::cont::Algorithm::Unique(uniqueSegmentIds);
    numSegments = uniqueSegmentIds.GetNumberOfValues();
    done = segmentIds.GetNumberOfValues() == 0;
    parentIndices = nextParentIndices;
    //PRINT_TIMER("5.1", s51);
    //std::cout << "Iteration time: " << iterationTimer.GetElapsedTime() << "\n";
  }
  //std::cout << "Total time: " << totalTimer.GetElapsedTime() << "\n";
}

namespace
{

struct CellLocatorBIHPrepareForExecutionFunctor
{
  template <typename DeviceAdapter, typename CellSetType>
  bool operator()(
    DeviceAdapter,
    const CellSetType& cellset,
    vtkm::cont::VirtualObjectHandle<vtkm::exec::CellLocator>& bihExec,
    const vtkm::cont::ArrayHandle<vtkm::exec::CellLocatorBoundingIntervalHierarchyNode>& nodes,
    const vtkm::cont::ArrayHandle<vtkm::Id>& processedCellIds,
    const vtkm::cont::ArrayHandleVirtualCoordinates& coords) const
  {
    using ExecutionType =
      vtkm::exec::CellLocatorBoundingIntervalHierarchyExec<DeviceAdapter, CellSetType>;
    ExecutionType* execObject =
      new ExecutionType(nodes, processedCellIds, cellset, coords, DeviceAdapter());
    bihExec.Reset(execObject);
    return true;
  }
};

struct BIHCellSetCaster
{
  template <typename CellSet, typename... Args>
  void operator()(CellSet&& cellset, vtkm::cont::DeviceAdapterId device, Args&&... args) const
  {
    //We need to go though CastAndCall first
    const bool success = vtkm::cont::TryExecuteOnDevice(
      device, CellLocatorBIHPrepareForExecutionFunctor(), cellset, std::forward<Args>(args)...);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("BoundingIntervalHierarchy", device);
    }
  }
};
}


const vtkm::exec::CellLocator* CellLocatorBoundingIntervalHierarchy::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device) const
{
  this->GetCellSet().CastAndCall(BIHCellSetCaster{},
                                 device,
                                 this->ExecutionObjectHandle,
                                 this->Nodes,
                                 this->ProcessedCellIds,
                                 this->GetCoordinates().GetData());
  return this->ExecutionObjectHandle.PrepareForExecution(device);
  ;
}

} //namespace cont
} //namespace vtkm
