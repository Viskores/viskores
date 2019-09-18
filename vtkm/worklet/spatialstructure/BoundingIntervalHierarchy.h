//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_spatialstructure_BoundingIntervalHierarchy_h
#define vtk_m_worklet_spatialstructure_BoundingIntervalHierarchy_h

#include <type_traits>

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
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/exec/CellLocatorBoundingIntervalHierarchyExec.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace worklet
{
namespace spatialstructure
{

struct TreeNode
{
  vtkm::FloatDefault LMax;
  vtkm::FloatDefault RMin;
  vtkm::IdComponent Dimension;

  VTKM_EXEC
  TreeNode()
    : LMax()
    , RMin()
    , Dimension()
  {
  }
}; // struct TreeNode

struct SplitProperties
{
  vtkm::FloatDefault Plane;
  vtkm::Id NumLeftPoints;
  vtkm::Id NumRightPoints;
  vtkm::FloatDefault LMax;
  vtkm::FloatDefault RMin;
  vtkm::FloatDefault Cost;

  VTKM_EXEC
  SplitProperties()
    : Plane()
    , NumLeftPoints()
    , NumRightPoints()
    , LMax()
    , RMin()
    , Cost()
  {
  }
}; // struct SplitProperties

struct CellRangesExtracter : public vtkm::worklet::WorkletVisitCellsWithPoints
{
  typedef void ControlSignature(CellSetIn,
                                WholeArrayIn,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell);
  typedef void ExecutionSignature(_1, PointIndices, _2, _3, _4, _5, _6, _7, _8);

  template <typename CellShape, typename PointIndicesVec, typename PointsPortal>
  VTKM_EXEC void operator()(CellShape vtkmNotUsed(shape),
                            const PointIndicesVec& pointIndices,
                            const PointsPortal& points,
                            vtkm::Range& rangeX,
                            vtkm::Range& rangeY,
                            vtkm::Range& rangeZ,
                            vtkm::FloatDefault& centerX,
                            vtkm::FloatDefault& centerY,
                            vtkm::FloatDefault& centerZ) const
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
    vtkm::Vec3f center = bounds.Center();
    centerX = center[0];
    centerY = center[1];
    centerZ = center[2];
  }
}; // struct CellRangesExtracter

struct LEQWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldOut, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  VTKM_EXEC
  void operator()(const vtkm::FloatDefault& value,
                  const vtkm::FloatDefault& planeValue,
                  vtkm::Id& leq,
                  vtkm::Id& r) const
  {
    leq = value <= planeValue;
    r = !leq;
  }
}; // struct LEQWorklet

template <bool LEQ>
struct FilterRanges;

template <>
struct FilterRanges<true> : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  VTKM_EXEC
  void operator()(const vtkm::FloatDefault& value,
                  const vtkm::FloatDefault& planeValue,
                  const vtkm::Range& cellBounds,
                  vtkm::Range& outBounds) const
  {
    outBounds = (value <= planeValue) ? cellBounds : vtkm::Range();
  }
}; // struct FilterRanges

template <>
struct FilterRanges<false> : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  VTKM_EXEC
  void operator()(const vtkm::FloatDefault& value,
                  const vtkm::FloatDefault& planeValue,
                  const vtkm::Range& cellBounds,
                  vtkm::Range& outBounds) const
  {
    outBounds = (value > planeValue) ? cellBounds : vtkm::Range();
  }
}; // struct FilterRanges

struct SplitPlaneCalculatorWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2);
  using InputDomain = _1;

  VTKM_CONT
  SplitPlaneCalculatorWorklet(vtkm::IdComponent planeIdx, vtkm::IdComponent numPlanes)
    : Scale(static_cast<vtkm::FloatDefault>(planeIdx + 1) /
            static_cast<vtkm::FloatDefault>(numPlanes + 1))
  {
  }

  VTKM_EXEC
  void operator()(const vtkm::Range& range, vtkm::FloatDefault& splitPlane) const
  {
    splitPlane = static_cast<vtkm::FloatDefault>(range.Min + Scale * (range.Max - range.Min));
  }

  vtkm::FloatDefault Scale;
};

struct SplitPropertiesCalculator : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, InputIndex);
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
                            const vtkm::FloatDefault& planeValue,
                            SplitPropertiesPortal& splits,
                            vtkm::Id inputIndex) const
  {
    SplitProperties split;
    split.Plane = planeValue;
    split.NumLeftPoints = pointsToLeft;
    split.NumRightPoints = pointsToRight;
    split.LMax = static_cast<vtkm::FloatDefault>(lMaxRanges.Max);
    split.RMin = static_cast<vtkm::FloatDefault>(rMinRanges.Min);
    split.Cost = vtkm::Abs(split.LMax * static_cast<vtkm::FloatDefault>(pointsToLeft) -
                           split.RMin * static_cast<vtkm::FloatDefault>(pointsToRight));
    if (vtkm::IsNan(split.Cost))
    {
      split.Cost = vtkm::Infinity<vtkm::FloatDefault>();
    }
    splits.Set(inputIndex * Stride + Index, split);
    //printf("Plane = %lf, NL = %lld, NR = %lld, LM = %lf, RM = %lf, C = %lf\n", split.Plane, split.NumLeftPoints, split.NumRightPoints, split.LMax, split.RMin, split.Cost);
  }

  vtkm::IdComponent Index;
  vtkm::Id Stride;
};

struct SplitSelector : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut);
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
                            vtkm::FloatDefault& plane,
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
    vtkm::FloatDefault minCost = vtkm::Infinity<vtkm::FloatDefault>();
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
    //printf("Selected plane %lf, with cost %lf [%d, %lf, %lf]\n", plane, minCost, node.Dimension, node.LMax, node.RMin);
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
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  VTKM_EXEC
  void operator()(const vtkm::FloatDefault& x,
                  const vtkm::FloatDefault& y,
                  const vtkm::FloatDefault& z,
                  const TreeNode& split,
                  const vtkm::FloatDefault& plane,
                  vtkm::Id& flag) const
  {
    if (split.Dimension >= 0)
    {
      const vtkm::Vec3f point(x, y, z);
      const vtkm::FloatDefault& c = point[split.Dimension];
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
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  VTKM_CONT
  SegmentSplitter(vtkm::IdComponent maxLeafSize)
    : MaxLeafSize(maxLeafSize)
  {
  }

  VTKM_EXEC
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
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);
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
  typedef void ControlSignature(FieldIn, FieldIn, WholeArrayOut);
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
  typedef void ControlSignature(FieldIn, FieldOut);
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
  typedef void ControlSignature(FieldIn nodeIndices,
                                FieldIn segmentSplits,
                                FieldIn nonSplitSegmentIndices,
                                FieldIn segmentSizes,
                                FieldIn runningSplitSegmentCounts,
                                FieldIn parentIndices,
                                WholeArrayInOut newTree,
                                WholeArrayOut nextParentIndices);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  VTKM_CONT
  TreeLevelAdder(vtkm::Id cellIdsOffset, vtkm::Id treeOffset, vtkm::IdComponent maxLeafSize)
    : CellIdsOffset(cellIdsOffset)
    , TreeOffset(treeOffset)
    , MaxLeafSize(maxLeafSize)
  {
  }

  template <typename BoundingIntervalHierarchyPortal, typename NextParentPortal>
  VTKM_EXEC void operator()(vtkm::Id index,
                            const TreeNode& split,
                            vtkm::Id start,
                            vtkm::Id count,
                            vtkm::Id numPreviousSplits,
                            vtkm::Id parentIndex,
                            BoundingIntervalHierarchyPortal& treePortal,
                            NextParentPortal& nextParentPortal) const
  {
    vtkm::exec::CellLocatorBoundingIntervalHierarchyNode node;
    node.ParentIndex = parentIndex;
    if (count > this->MaxLeafSize)
    {
      node.Dimension = split.Dimension;
      node.ChildIndex = this->TreeOffset + 2 * numPreviousSplits;
      node.Node.LMax = split.LMax;
      node.Node.RMin = split.RMin;
      nextParentPortal.Set(2 * numPreviousSplits, index);
      nextParentPortal.Set(2 * numPreviousSplits + 1, index);
    }
    else
    {
      node.ChildIndex = -1;
      node.Leaf.Start = this->CellIdsOffset + start;
      node.Leaf.Size = count;
    }
    treePortal.Set(index, node);
  }

  vtkm::Id CellIdsOffset;
  vtkm::Id TreeOffset;
  vtkm::IdComponent MaxLeafSize;
}; // struct TreeLevelAdder

template <typename T, class BinaryFunctor>
vtkm::cont::ArrayHandle<T> ReverseScanInclusiveByKey(const vtkm::cont::ArrayHandle<T>& keys,
                                                     const vtkm::cont::ArrayHandle<T>& values,
                                                     BinaryFunctor binaryFunctor)
{
  vtkm::cont::ArrayHandle<T> result;
  auto reversedResult = vtkm::cont::make_ArrayHandleReverse(result);

  vtkm::cont::Algorithm::ScanInclusiveByKey(vtkm::cont::make_ArrayHandleReverse(keys),
                                            vtkm::cont::make_ArrayHandleReverse(values),
                                            reversedResult,
                                            binaryFunctor);

  return result;
}

template <typename T, typename U>
vtkm::cont::ArrayHandle<T> CopyIfArray(const vtkm::cont::ArrayHandle<T>& input,
                                       const vtkm::cont::ArrayHandle<U>& stencil)
{
  vtkm::cont::ArrayHandle<T> result;
  vtkm::cont::Algorithm::CopyIf(input, stencil, result);

  return result;
}

VTKM_CONT
struct Invert
{
  VTKM_EXEC
  vtkm::Id operator()(const vtkm::Id& value) const { return 1 - value; }
}; // struct Invert

VTKM_CONT
struct RangeAdd
{
  VTKM_EXEC
  vtkm::Range operator()(const vtkm::Range& accumulator, const vtkm::Range& value) const
  {
    if (value.IsNonEmpty())
    {
      return accumulator.Union(value);
    }
    else
    {
      return accumulator;
    }
  }
}; // struct RangeAdd

} // namespace spatialstructure
} // namespace worklet
} // namespace vtkm

#endif //vtk_m_worklet_spatialstructure_BoundingIntervalHierarchy_h
