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

#ifndef vtk_m_worklet_spatialstructure_BoundaryIntervalHierarchyBuilder_h
#define vtk_m_worklet_spatialstructure_BoundaryIntervalHierarchyBuilder_h

#include <vtkm/Bounds.h>
#include <vtkm/Types.h>
#include <vtkm/VecFromPortalPermute.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleReverse.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/spatialstructure/BoundaryIntervalHierarchy.h>
#include <vtkm/worklet/spatialstructure/BoundaryIntervalHierarchyNode.h>

namespace vtkm
{
namespace worklet
{
namespace spatialstructure
{
namespace
{

struct TreeNode
{
  vtkm::Float64 LMax;
  vtkm::Float64 RMin;
  vtkm::IdComponent Dimension;
}; // struct TreeNode

struct SplitProperties
{
  vtkm::Float64 Plane;
  vtkm::Id NumLeftPoints;
  vtkm::Id NumRightPoints;
  vtkm::Float64 LMax;
  vtkm::Float64 RMin;
  vtkm::Float64 Cost;
}; // struct SplitProperties

struct CellRangesExtracter : public vtkm::worklet::WorkletMapPointToCell
{
  typedef void ControlSignature(CellSetIn,
                                WholeArrayIn<>,
                                FieldOutCell<>,
                                FieldOutCell<>,
                                FieldOutCell<>,
                                FieldOutCell<>,
                                FieldOutCell<>,
                                FieldOutCell<>);
  typedef void ExecutionSignature(_1, PointIndices, _2, _3, _4, _5, _6, _7, _8);

  template <typename CellShape, typename PointIndicesVec, typename PointsPortal>
  VTKM_EXEC void operator()(CellShape vtkmNotUsed(shape),
                            const PointIndicesVec& pointIndices,
                            const PointsPortal& points,
                            vtkm::Range& rangeX,
                            vtkm::Range& rangeY,
                            vtkm::Range& rangeZ,
                            vtkm::Float64& centerX,
                            vtkm::Float64& centerY,
                            vtkm::Float64& centerZ) const
  {
    vtkm::Bounds bounds;
    vtkm::VecFromPortalPermute<PointIndicesVec, PointsPortal> cellPoints(&pointIndices, points);
    vtkm::IdComponent numPoints = cellPoints.GetNumberOfComponents();
    for (vtkm::IdComponent i = 0; i < numPoints; ++i)
    {
      bounds.Include(cellPoints[i]);
    }
    rangeX = bounds.X;
    rangeY = bounds.Y;
    rangeZ = bounds.Z;
    vtkm::Vec<vtkm::Float64, 3> center = bounds.Center();
    centerX = center[0];
    centerY = center[1];
    centerZ = center[2];
  }
}; // struct CellRangesExtracter

struct LEQWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldOut<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  VTKM_EXEC
  void operator()(const vtkm::Float64& value,
                  const vtkm::Float64& planeValue,
                  vtkm::Id& leq,
                  vtkm::Id& r) const
  {
    leq = value <= planeValue;
    r = !leq;
  }
}; // struct LEQWorklet

template <bool LEQ>
struct FilterRanges : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldIn<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  VTKM_EXEC
  void operator()(const vtkm::Float64& value,
                  const vtkm::Float64& planeValue,
                  const vtkm::Range& cellBounds,
                  vtkm::Range& outBounds) const
  {
    if (LEQ)
    {
      outBounds = (value <= planeValue) ? cellBounds : vtkm::Range(0.0, 0.0);
    }
    else
    {
      outBounds = (value > planeValue) ? cellBounds : vtkm::Range(0.0, 0.0);
    }
  }
}; // struct FilterRanges

struct SplitPlaneCalculatorWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2);
  using InputDomain = _1;

  VTKM_CONT
  SplitPlaneCalculatorWorklet(vtkm::IdComponent planeIdx, vtkm::IdComponent numPlanes)
    : Scale(static_cast<vtkm::Float64>(planeIdx + 1) / static_cast<vtkm::Float64>(numPlanes + 1))
  {
  }

  VTKM_EXEC
  void operator()(const vtkm::Range& range, vtkm::Float64& splitPlane) const
  {
    splitPlane = range.Min + Scale * (range.Max - range.Min);
  }

  vtkm::Float64 Scale;
};

struct SplitPropertiesCalculator : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature =
    void(FieldIn<>, FieldIn<>, FieldIn<>, FieldIn<>, FieldIn<>, WholeArrayInOut<>);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, InputIndex);
  using InputDomain = _1;

  VTKM_CONT
  SplitPropertiesCalculator(vtkm::IdComponent index, vtkm::Id stride)
    : Index(index)
    , Stride(stride)
  {
  }

  template <typename SplitPropertiesPortal>
  VTKM_EXEC void operator()(const vtkm::Id& pointsToLeft,
                            const vtkm::Id& pointsToRight,
                            const vtkm::Range& lMaxRanges,
                            const vtkm::Range& rMinRanges,
                            const vtkm::Float64& planeValue,
                            SplitPropertiesPortal& splits,
                            vtkm::Id inputIndex) const
  {
    SplitProperties split;
    split.Plane = planeValue;
    split.NumLeftPoints = pointsToLeft;
    split.NumRightPoints = pointsToRight;
    split.LMax = lMaxRanges.Max;
    split.RMin = rMinRanges.Min;
    split.Cost = vtkm::Abs(split.LMax * static_cast<vtkm::Float64>(pointsToLeft) -
                           split.RMin * static_cast<vtkm::Float64>(pointsToRight));
    splits.Set(inputIndex * Stride + Index, split);
  }

  vtkm::IdComponent Index;
  vtkm::Id Stride;
};

struct SplitSelector : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<>,
                                WholeArrayIn<>,
                                WholeArrayIn<>,
                                WholeArrayIn<>,
                                FieldIn<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  VTKM_CONT
  SplitSelector(vtkm::IdComponent numPlanes,
                vtkm::IdComponent maxLeafSize,
                vtkm::IdComponent stride)
    : NumPlanes(numPlanes)
    , MaxLeafSize(maxLeafSize)
    , Stride(stride)
  {
  }

  template <typename SplitPropertiesPortal>
  VTKM_EXEC void operator()(vtkm::Id index,
                            const SplitPropertiesPortal& xSplits,
                            const SplitPropertiesPortal& ySplits,
                            const SplitPropertiesPortal& zSplits,
                            const vtkm::Id& segmentSize,
                            TreeNode& node,
                            vtkm::Float64& plane,
                            vtkm::Id& choice) const
  {
    if (segmentSize <= MaxLeafSize)
    {
      node.Dimension = -1;
      choice = 0;
      return;
    }
    choice = 1;
    using Split = SplitProperties;
    vtkm::Float64 minCost = vtkm::Infinity64();
    const Split& xSplit = xSplits[ArgMin(xSplits, index * Stride, Stride)];
    bool found = false;
    if (xSplit.Cost < minCost && xSplit.NumLeftPoints != 0 && xSplit.NumRightPoints != 0)
    {
      minCost = xSplit.Cost;
      node.Dimension = 0;
      node.LMax = xSplit.LMax;
      node.RMin = xSplit.RMin;
      plane = xSplit.Plane;
      found = true;
    }
    const Split& ySplit = ySplits[ArgMin(ySplits, index * Stride, Stride)];
    if (ySplit.Cost < minCost && ySplit.NumLeftPoints != 0 && ySplit.NumRightPoints != 0)
    {
      minCost = ySplit.Cost;
      node.Dimension = 1;
      node.LMax = ySplit.LMax;
      node.RMin = ySplit.RMin;
      plane = ySplit.Plane;
      found = true;
    }
    const Split& zSplit = zSplits[ArgMin(zSplits, index * Stride, Stride)];
    if (zSplit.Cost < minCost && zSplit.NumLeftPoints != 0 && zSplit.NumRightPoints != 0)
    {
      minCost = zSplit.Cost;
      node.Dimension = 2;
      node.LMax = zSplit.LMax;
      node.RMin = zSplit.RMin;
      plane = zSplit.Plane;
      found = true;
    }
    if (!found)
    {
      const Split& xMSplit = xSplits[NumPlanes];
      minCost = xMSplit.Cost;
      node.Dimension = 0;
      node.LMax = xMSplit.LMax;
      node.RMin = xMSplit.RMin;
      plane = xMSplit.Plane;
      const Split& yMSplit = ySplits[NumPlanes];
      if (yMSplit.Cost < minCost && yMSplit.NumLeftPoints != 0 && yMSplit.NumRightPoints != 0)
      {
        minCost = yMSplit.Cost;
        node.Dimension = 1;
        node.LMax = yMSplit.LMax;
        node.RMin = yMSplit.RMin;
        plane = yMSplit.Plane;
      }
      const Split& zMSplit = zSplits[NumPlanes];
      if (zMSplit.Cost < minCost && zMSplit.NumLeftPoints != 0 && zMSplit.NumRightPoints != 0)
      {
        minCost = zMSplit.Cost;
        node.Dimension = 2;
        node.LMax = zMSplit.LMax;
        node.RMin = zMSplit.RMin;
        plane = zMSplit.Plane;
      }
    }
  }

  template <typename ArrayPortal>
  VTKM_EXEC vtkm::Id ArgMin(const ArrayPortal& values, vtkm::Id start, vtkm::Id length) const
  {
    vtkm::Id minIdx = start;
    for (vtkm::Id i = start; i < (start + length); ++i)
    {
      if (values[i].Cost < values[minIdx].Cost)
      {
        minIdx = i;
      }
    }
    return minIdx;
  }

  vtkm::IdComponent NumPlanes;
  vtkm::IdComponent MaxLeafSize;
  vtkm::Id Stride;
};

struct CalculateSplitDirectionFlag : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldIn<>, FieldIn<>, FieldIn<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  void operator()(const vtkm::Float64& x,
                  const vtkm::Float64& y,
                  const vtkm::Float64& z,
                  const TreeNode& split,
                  const vtkm::Float64& plane,
                  vtkm::Id& flag) const
  {
    if (split.Dimension >= 0)
    {
      const vtkm::Vec<vtkm::Float64, 3> point(x, y, z);
      const vtkm::Float64& c = point[split.Dimension];
      // We use 0 to signify left child, 1 for right child
      flag = 1 - static_cast<vtkm::Id>(c <= plane);
    }
    else
    {
      flag = 0;
    }
  }
}; // struct CalculateSplitDirectionFlag

struct SegmentSplitter : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldIn<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  VTKM_CONT
  SegmentSplitter(vtkm::IdComponent maxLeafSize)
    : MaxLeafSize(maxLeafSize)
  {
  }

  void operator()(const vtkm::Id& segmentId,
                  const vtkm::Id& leqFlag,
                  const vtkm::Id& segmentSize,
                  vtkm::Id& newSegmentId) const
  {
    if (segmentSize <= MaxLeafSize)
    {
      // We do not split the segments which have cells fewer than MaxLeafSize, moving them to left
      newSegmentId = 2 * segmentId;
    }
    else
    {
      newSegmentId = 2 * segmentId + leqFlag;
    }
  }

  vtkm::IdComponent MaxLeafSize;
}; // struct SegmentSplitter

struct SplitIndicesCalculator : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldIn<>, FieldIn<>, FieldIn<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  VTKM_EXEC
  void operator()(const vtkm::Id& leqFlag,
                  const vtkm::Id& trueFlagCount,
                  const vtkm::Id& countPreviousSegment,
                  const vtkm::Id& runningFalseFlagCount,
                  const vtkm::Id& totalFalseFlagCount,
                  vtkm::Id& scatterIndex) const
  {
    if (leqFlag)
    {
      scatterIndex = countPreviousSegment + totalFalseFlagCount + trueFlagCount;
    }
    else
    {
      scatterIndex = countPreviousSegment + runningFalseFlagCount - 1;
    }
  }
}; // struct SplitIndicesCalculator

struct Scatter : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<>, FieldIn<>, WholeArrayOut<>);
  typedef void ExecutionSignature(_1, _2, _3);
  using InputDomain = _1;

  template <typename InputType, typename OutputPortalType>
  VTKM_EXEC void operator()(const InputType& in, const vtkm::Id& idx, OutputPortalType& out) const
  {
    out.Set(idx, in);
  }
}; // struct Scatter

template <typename ValueArrayHandle, typename IndexArrayHandle>
ValueArrayHandle ScatterArray(const ValueArrayHandle& input, const IndexArrayHandle& indices)
{
  ValueArrayHandle output;
  output.Allocate(input.GetNumberOfValues());
  vtkm::worklet::DispatcherMapField<Scatter>().Invoke(input, indices, output);
  return output;
}

struct NonSplitIndexCalculator : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2);
  using InputDomain = _1;

  VTKM_CONT
  NonSplitIndexCalculator(vtkm::IdComponent maxLeafSize)
    : MaxLeafSize(maxLeafSize)
  {
  }

  VTKM_EXEC void operator()(const vtkm::Id& inSegmentSize, vtkm::Id& outSegmentSize) const
  {
    if (inSegmentSize <= MaxLeafSize)
    {
      outSegmentSize = inSegmentSize;
    }
    else
    {
      outSegmentSize = 0;
    }
  }

  vtkm::Id MaxLeafSize;
}; // struct NonSplitIndexCalculator

struct TreeLevelAdder : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<>,
                                FieldIn<>,
                                FieldIn<>,
                                FieldIn<>,
                                FieldIn<>,
                                WholeArrayInOut<>);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  VTKM_CONT
  TreeLevelAdder(vtkm::Id cellIdsOffset, vtkm::Id treeOffset, vtkm::IdComponent maxLeafSize)
    : CellIdsOffset(cellIdsOffset)
    , TreeOffset(treeOffset)
    , MaxLeafSize(maxLeafSize)
  {
  }

  template <typename BoundaryIntervalHierarchyPortal>
  VTKM_EXEC void operator()(const vtkm::Id& index,
                            const TreeNode& split,
                            const vtkm::Id& start,
                            const vtkm::Id& count,
                            const vtkm::Id& numPreviousSplits,
                            BoundaryIntervalHierarchyPortal& treePortal) const
  {
    BoundaryIntervalHierarchyNode node;
    if (count > MaxLeafSize)
    {
      node.Dimension = split.Dimension;
      node.ChildIndex = TreeOffset + 2 * numPreviousSplits;
      node.Node.LMax = split.LMax;
      node.Node.RMin = split.RMin;
    }
    else
    {
      node.ChildIndex = -1;
      node.Leaf.Start = CellIdsOffset + start;
      node.Leaf.Size = count;
    }
    treePortal.Set(index, node);
  }

  vtkm::Id CellIdsOffset;
  vtkm::Id TreeOffset;
  vtkm::IdComponent MaxLeafSize;
}; // struct TreeLevelAdder

template <typename T, class BinaryFunctor, typename DeviceAdapter>
vtkm::cont::ArrayHandle<T> ReverseScanInclusiveByKey(const vtkm::cont::ArrayHandle<T>& keys,
                                                     const vtkm::cont::ArrayHandle<T>& values,
                                                     BinaryFunctor binaryFunctor,
                                                     DeviceAdapter)
{
  using Algorithms = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  vtkm::cont::ArrayHandle<T> result;
  auto reversedResult = vtkm::cont::make_ArrayHandleReverse(result);

  Algorithms::ScanInclusiveByKey(vtkm::cont::make_ArrayHandleReverse(keys),
                                 vtkm::cont::make_ArrayHandleReverse(values),
                                 reversedResult,
                                 binaryFunctor);

  return result;
}

template <typename T, typename U, typename DeviceAdapter>
vtkm::cont::ArrayHandle<T> CopyIfArray(const vtkm::cont::ArrayHandle<T>& input,
                                       const vtkm::cont::ArrayHandle<U>& stencil,
                                       DeviceAdapter)
{
  using Algorithms = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  vtkm::cont::ArrayHandle<T> result;
  Algorithms::CopyIf(input, stencil, result);

  return result;
}

} // namespace

class BoundaryIntervalHierarchyBuilder
{
private:
  using IdArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
  using IdPermutationArrayHandle = vtkm::cont::ArrayHandlePermutation<IdArrayHandle, IdArrayHandle>;
  using BoundsArrayHandle = vtkm::cont::ArrayHandle<vtkm::Bounds>;
  using CoordsArrayHandle = vtkm::cont::ArrayHandle<vtkm::Float64>;
  using CoordsPermutationArrayHandle =
    vtkm::cont::ArrayHandlePermutation<IdArrayHandle, CoordsArrayHandle>;
  using CountingIdArrayHandle = vtkm::cont::ArrayHandleCounting<vtkm::Id>;
  using RangeArrayHandle = vtkm::cont::ArrayHandle<vtkm::Range>;
  using RangePermutationArrayHandle =
    vtkm::cont::ArrayHandlePermutation<IdArrayHandle, RangeArrayHandle>;
  using SplitArrayHandle = vtkm::cont::ArrayHandle<TreeNode>;
  using SplitPermutationArrayHandle =
    vtkm::cont::ArrayHandlePermutation<IdArrayHandle, SplitArrayHandle>;
  using SplitPropertiesArrayHandle = vtkm::cont::ArrayHandle<SplitProperties>;

public:
  VTKM_CONT
  BoundaryIntervalHierarchyBuilder(vtkm::IdComponent numPlanes = 4,
                                   vtkm::IdComponent maxLeafSize = 5)
    : NumPlanes(numPlanes)
    , MaxLeafSize(maxLeafSize)
  {
  }

  VTKM_CONT
  template <typename CellSetType, typename PointArrayHandle, typename DeviceAdapter>
  BoundaryIntervalHierarchy Build(const CellSetType& cellSet,
                                  const PointArrayHandle& points,
                                  DeviceAdapter)
  {
    using Algorithms = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    vtkm::Id numCells = cellSet.GetNumberOfCells();
    IdArrayHandle cellIds;
    Algorithms::Copy(CountingIdArrayHandle(0, 1, numCells), cellIds);
    IdArrayHandle segmentIds;
    Algorithms::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, numCells), segmentIds);

    CoordsArrayHandle centerXs, centerYs, centerZs;
    RangeArrayHandle xRanges, yRanges, zRanges;
    vtkm::worklet::DispatcherMapTopology<CellRangesExtracter, DeviceAdapter>().Invoke(
      cellSet, points, xRanges, yRanges, zRanges, centerXs, centerYs, centerZs);

    bool done = false;
    vtkm::IdComponent iteration = 0;
    vtkm::cont::ArrayHandle<BoundaryIntervalHierarchyNode> nodes;
    vtkm::Id nodesIndexOffset = 0;
    vtkm::Id numSegments = 1;
    IdArrayHandle discardKeys;
    IdArrayHandle segmentStarts;
    IdArrayHandle segmentSizes;
    segmentSizes.Allocate(1);
    segmentSizes.GetPortalControl().Set(0, numCells);
    IdArrayHandle processedCellIds;
    processedCellIds.Allocate(numCells);
    vtkm::Id cellIdsOffset = 0;
    while (!done)
    {
      std::cout << "**** Iteration " << (++iteration) << " ****\n";

      // Calculate the X, Y, Z bounding ranges for each segment
      RangeArrayHandle perSegmentXRanges, perSegmentYRanges, perSegmentZRanges;
      Algorithms::ReduceByKey(segmentIds, xRanges, discardKeys, perSegmentXRanges, vtkm::Add());
      Algorithms::ReduceByKey(segmentIds, yRanges, discardKeys, perSegmentYRanges, vtkm::Add());
      Algorithms::ReduceByKey(segmentIds, zRanges, discardKeys, perSegmentZRanges, vtkm::Add());

      // Expand the per segment bounding ranges, to per cell;
      RangePermutationArrayHandle segmentXRanges(segmentIds, perSegmentXRanges);
      RangePermutationArrayHandle segmentYRanges(segmentIds, perSegmentYRanges);
      RangePermutationArrayHandle segmentZRanges(segmentIds, perSegmentZRanges);

      // Calculate split costs for NumPlanes split planes, across X, Y and Z dimensions
      vtkm::Id numSplitPlanes = numSegments * (NumPlanes + 1);
      vtkm::cont::ArrayHandle<SplitProperties> xSplits, ySplits, zSplits;
      xSplits.Allocate(numSplitPlanes);
      ySplits.Allocate(numSplitPlanes);
      zSplits.Allocate(numSplitPlanes);
      CalculateSplitCosts(segmentXRanges, xRanges, centerXs, segmentIds, xSplits, DeviceAdapter());
      CalculateSplitCosts(segmentYRanges, yRanges, centerYs, segmentIds, ySplits, DeviceAdapter());
      CalculateSplitCosts(segmentZRanges, zRanges, centerZs, segmentIds, zSplits, DeviceAdapter());

      // Select best split plane and dimension across X, Y, Z dimension, per segment
      SplitArrayHandle segmentSplits;
      vtkm::cont::ArrayHandle<vtkm::Float64> segmentPlanes;
      vtkm::cont::ArrayHandle<vtkm::Id> splitChoices;
      SplitSelector worklet(NumPlanes, MaxLeafSize, NumPlanes + 1);
      vtkm::worklet::DispatcherMapField<SplitSelector> splitSelectorDispatcher(worklet);
      CountingIdArrayHandle indices(0, 1, numSegments);
      splitSelectorDispatcher.Invoke(indices,
                                     xSplits,
                                     ySplits,
                                     zSplits,
                                     segmentSizes,
                                     segmentSplits,
                                     segmentPlanes,
                                     splitChoices);

      // Expand the per segment split plane to per cell
      SplitPermutationArrayHandle splits(segmentIds, segmentSplits);
      CoordsPermutationArrayHandle planes(segmentIds, segmentPlanes);

      IdArrayHandle leqFlags;
      vtkm::worklet::DispatcherMapField<CalculateSplitDirectionFlag> computeFlagDispatcher;
      computeFlagDispatcher.Invoke(centerXs, centerYs, centerZs, splits, planes, leqFlags);

      IdArrayHandle scatterIndices =
        CalculateSplitScatterIndices(cellIds, leqFlags, segmentIds, DeviceAdapter());
      IdArrayHandle newSegmentIds;
      IdPermutationArrayHandle sizes(segmentIds, segmentSizes);
      vtkm::worklet::DispatcherMapField<SegmentSplitter>(SegmentSplitter(MaxLeafSize))
        .Invoke(segmentIds, leqFlags, sizes, newSegmentIds);

      vtkm::cont::ArrayHandle<vtkm::Id> choices;
      Algorithms::Copy(IdPermutationArrayHandle(segmentIds, splitChoices), choices);
      cellIds = ScatterArray(cellIds, scatterIndices);
      segmentIds = ScatterArray(segmentIds, scatterIndices);
      newSegmentIds = ScatterArray(newSegmentIds, scatterIndices);
      xRanges = ScatterArray(xRanges, scatterIndices);
      yRanges = ScatterArray(yRanges, scatterIndices);
      zRanges = ScatterArray(zRanges, scatterIndices);
      centerXs = ScatterArray(centerXs, scatterIndices);
      centerYs = ScatterArray(centerYs, scatterIndices);
      centerZs = ScatterArray(centerZs, scatterIndices);
      choices = ScatterArray(choices, scatterIndices);

      // Move the cell ids at leafs to the processed cellids list
      IdArrayHandle nonSplitSegmentSizes;
      vtkm::worklet::DispatcherMapField<NonSplitIndexCalculator>(
        NonSplitIndexCalculator(MaxLeafSize))
        .Invoke(segmentSizes, nonSplitSegmentSizes);
      IdArrayHandle nonSplitSegmentIndices;
      Algorithms::ScanExclusive(nonSplitSegmentSizes, nonSplitSegmentIndices);
      IdArrayHandle runningSplitSegmentCounts;
      Algorithms::ScanExclusive(splitChoices, runningSplitSegmentCounts);

      IdArrayHandle doneCellIds;
      Algorithms::CopyIf(cellIds, choices, doneCellIds, Invert());
      Algorithms::CopySubRange(
        doneCellIds, 0, doneCellIds.GetNumberOfValues(), processedCellIds, cellIdsOffset);

      cellIds = CopyIfArray(cellIds, choices, DeviceAdapter());
      newSegmentIds = CopyIfArray(newSegmentIds, choices, DeviceAdapter());
      xRanges = CopyIfArray(xRanges, choices, DeviceAdapter());
      yRanges = CopyIfArray(yRanges, choices, DeviceAdapter());
      zRanges = CopyIfArray(zRanges, choices, DeviceAdapter());
      centerXs = CopyIfArray(centerXs, choices, DeviceAdapter());
      centerYs = CopyIfArray(centerYs, choices, DeviceAdapter());
      centerZs = CopyIfArray(centerZs, choices, DeviceAdapter());

      // Make a new nodes with enough nodes for the currnt level, copying over the old one
      vtkm::Id nodesSize = nodes.GetNumberOfValues() + numSegments;
      vtkm::cont::ArrayHandle<BoundaryIntervalHierarchyNode> newTree;
      newTree.Allocate(nodesSize);
      Algorithms::CopySubRange(nodes, 0, nodes.GetNumberOfValues(), newTree);

      CountingIdArrayHandle nodesIndices(nodesIndexOffset, 1, numSegments);
      TreeLevelAdder nodesAdder(cellIdsOffset, nodesSize, MaxLeafSize);
      vtkm::worklet::DispatcherMapField<TreeLevelAdder>(nodesAdder)
        .Invoke(nodesIndices,
                segmentSplits,
                nonSplitSegmentIndices,
                segmentSizes,
                runningSplitSegmentCounts,
                newTree);
      nodesIndexOffset = nodesSize;
      cellIdsOffset += doneCellIds.GetNumberOfValues();
      nodes = newTree;
      segmentIds = newSegmentIds;
      segmentSizes =
        CalculateSegmentSizes<DeviceAdapter>(segmentIds, segmentIds.GetNumberOfValues());
      segmentIds = GenerateSegmentIds<DeviceAdapter>(segmentSizes, segmentIds.GetNumberOfValues());
      IdArrayHandle uniqueSegmentIds;
      Algorithms::Copy(segmentIds, uniqueSegmentIds);
      Algorithms::Unique(uniqueSegmentIds);
      numSegments = uniqueSegmentIds.GetNumberOfValues();
      done = segmentIds.GetNumberOfValues() == 0;
    }
    return BoundaryIntervalHierarchy(nodes, processedCellIds);
  }

  template <typename DeviceAdapter>
  IdArrayHandle CalculateSegmentSizes(const IdArrayHandle& segmentIds, vtkm::Id numCells)
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
  IdArrayHandle GenerateSegmentIds(const IdArrayHandle& segmentSizes, vtkm::Id numCells)
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

  template <typename SegmentRangeArrayHandle, typename RangeArrayHandle, typename DeviceAdapter>
  void CalculateSplitCosts(SegmentRangeArrayHandle segmentRanges,
                           RangeArrayHandle ranges,
                           CoordsArrayHandle& coords,
                           IdArrayHandle& segmentIds,
                           vtkm::cont::ArrayHandle<SplitProperties>& splits,
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

  template <typename SegmentRangeArrayHandle, typename RangeArrayHandle, typename DeviceAdapter>
  void CalculatePlaneSplitCost(vtkm::IdComponent planeIndex,
                               vtkm::IdComponent numPlanes,
                               SegmentRangeArrayHandle segmentRanges,
                               RangeArrayHandle ranges,
                               CoordsArrayHandle& coords,
                               IdArrayHandle& segmentIds,
                               vtkm::cont::ArrayHandle<SplitProperties>& splits,
                               vtkm::IdComponent index,
                               DeviceAdapter)
  {
    using Algorithms = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    // Make candidate split plane array
    vtkm::cont::ArrayHandle<vtkm::Float64> splitPlanes;
    SplitPlaneCalculatorWorklet splitPlaneCalcWorklet(planeIndex, numPlanes);
    vtkm::worklet::DispatcherMapField<SplitPlaneCalculatorWorklet, DeviceAdapter> splitDispatcher(
      splitPlaneCalcWorklet);
    splitDispatcher.Invoke(segmentRanges, splitPlanes);

    // Check if a point is to the left of the split plane or right
    vtkm::cont::ArrayHandle<vtkm::Id> isLEQOfSplitPlane, isROfSplitPlane;
    vtkm::worklet::DispatcherMapField<LEQWorklet, DeviceAdapter>().Invoke(
      coords, splitPlanes, isLEQOfSplitPlane, isROfSplitPlane);

    // Count of points to the left
    vtkm::cont::ArrayHandle<vtkm::Id> pointsToLeft;
    IdArrayHandle discardKeys;
    Algorithms::ReduceByKey(segmentIds, isLEQOfSplitPlane, discardKeys, pointsToLeft, vtkm::Add());

    // Count of points to the right
    vtkm::cont::ArrayHandle<vtkm::Id> pointsToRight;
    Algorithms::ReduceByKey(segmentIds, isROfSplitPlane, discardKeys, pointsToRight, vtkm::Add());

    // Calculate Lmax and Rmin
    vtkm::cont::ArrayHandle<vtkm::Range> leqRanges;
    vtkm::cont::ArrayHandle<vtkm::Range> rRanges;
    vtkm::worklet::DispatcherMapField<FilterRanges<true>>().Invoke(
      coords, splitPlanes, ranges, leqRanges);
    vtkm::worklet::DispatcherMapField<FilterRanges<false>>().Invoke(
      coords, splitPlanes, ranges, rRanges);

    vtkm::cont::ArrayHandle<vtkm::Range> lMaxRanges;
    vtkm::cont::ArrayHandle<vtkm::Range> rMinRanges;
    Algorithms::ReduceByKey(segmentIds, leqRanges, discardKeys, lMaxRanges, vtkm::Add());
    Algorithms::ReduceByKey(segmentIds, rRanges, discardKeys, rMinRanges, vtkm::Add());

    vtkm::cont::ArrayHandle<vtkm::Float64> segmentedSplitPlanes;
    Algorithms::ReduceByKey(
      segmentIds, splitPlanes, discardKeys, segmentedSplitPlanes, vtkm::Minimum());

    // Calculate costs
    SplitPropertiesCalculator splitPropertiesCalculator(index, NumPlanes + 1);
    vtkm::worklet::DispatcherMapField<SplitPropertiesCalculator>(splitPropertiesCalculator)
      .Invoke(pointsToLeft, pointsToRight, lMaxRanges, rMinRanges, segmentedSplitPlanes, splits);
  }

  template <typename DeviceAdapter>
  IdArrayHandle CalculateSplitScatterIndices(const IdArrayHandle& cellIds,
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
    vtkm::cont::ArrayHandleTransform<IdArrayHandle, Invert> flagsInverse(leqFlags, Invert());
    vtkm::cont::ArrayHandle<vtkm::Id> runningFalseFlagCount;
    Algorithms::ScanInclusiveByKey(segmentIds, flagsInverse, runningFalseFlagCount, vtkm::Add());

    // Total number of false flags in segment
    IdArrayHandle totalFalseFlagSegmentCount = ReverseScanInclusiveByKey(
      segmentIds, runningFalseFlagCount, vtkm::Maximum(), DeviceAdapter());

    // if point is to the left,
    //    index = total number in  previous segments + total number of false flags in this segment + total number of trues in previous segment
    // else
    //    index = total number in previous segments + number of falses preceeding it in the segment.
    IdArrayHandle scatterIndices;
    vtkm::worklet::DispatcherMapField<SplitIndicesCalculator>().Invoke(leqFlags,
                                                                       trueFlagCounts,
                                                                       countPreviousSegments,
                                                                       runningFalseFlagCount,
                                                                       totalFalseFlagSegmentCount,
                                                                       scatterIndices);
    return scatterIndices;
  }

  VTKM_EXEC
  struct Invert
  {
    VTKM_EXEC
    vtkm::Id operator()(const vtkm::Id& value) const { return 1 - value; }
  };

private:
  vtkm::IdComponent NumPlanes;
  vtkm::IdComponent MaxLeafSize;
};
}
}
} // namespace vtkm::worklet::spatialstructure

#endif // vtk_m_worklet_spatialstructure_BoundaryIntervalHierarchyBuilder_h
