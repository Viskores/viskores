//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_m_worklet_Clip_h
#define viskores_m_worklet_Clip_h

#include <viskores/filter/clean_grid/worklet/RemoveUnusedPoints.h>
#include <viskores/filter/contour/worklet/clip/ClipTables.h>
#include <viskores/worklet/Keys.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>
#include <viskores/worklet/WorkletReduceByKey.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/Timer.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/ImplicitFunction.h>

#include <utility>
#include <viskores/exec/FunctorBase.h>

#if defined(THRUST_MAJOR_VERSION) && THRUST_MAJOR_VERSION == 1 && THRUST_MINOR_VERSION == 8 && \
  THRUST_SUBMINOR_VERSION < 3
// Workaround a bug in thrust 1.8.0 - 1.8.2 scan implementations which produces
// wrong results
#include <viskores/exec/cuda/internal/ThrustPatches.h>
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <thrust/detail/type_traits.h>
VISKORES_THIRDPARTY_POST_INCLUDE
#define THRUST_SCAN_WORKAROUND
#endif

namespace viskores
{
namespace worklet
{
struct ClipStats
{
  viskores::Id NumberOfCells = 0;
  viskores::Id NumberOfIndices = 0;
  viskores::Id NumberOfEdgeIndices = 0;

  // Stats for interpolating new points within cell.
  viskores::Id NumberOfInCellPoints = 0;
  viskores::Id NumberOfInCellIndices = 0;
  viskores::Id NumberOfInCellInterpPoints = 0;
  viskores::Id NumberOfInCellEdgeIndices = 0;

  struct SumOp
  {
    VISKORES_EXEC_CONT
    ClipStats operator()(const ClipStats& stat1, const ClipStats& stat2) const
    {
      ClipStats sum = stat1;
      sum.NumberOfCells += stat2.NumberOfCells;
      sum.NumberOfIndices += stat2.NumberOfIndices;
      sum.NumberOfEdgeIndices += stat2.NumberOfEdgeIndices;
      sum.NumberOfInCellPoints += stat2.NumberOfInCellPoints;
      sum.NumberOfInCellIndices += stat2.NumberOfInCellIndices;
      sum.NumberOfInCellInterpPoints += stat2.NumberOfInCellInterpPoints;
      sum.NumberOfInCellEdgeIndices += stat2.NumberOfInCellEdgeIndices;
      return sum;
    }
  };
};

struct EdgeInterpolation
{
  viskores::Id Vertex1 = -1;
  viskores::Id Vertex2 = -1;
  viskores::Float64 Weight = 0;

  struct LessThanOp
  {
    VISKORES_EXEC
    bool operator()(const EdgeInterpolation& v1, const EdgeInterpolation& v2) const
    {
      return (v1.Vertex1 < v2.Vertex1) || (v1.Vertex1 == v2.Vertex1 && v1.Vertex2 < v2.Vertex2);
    }
  };

  struct EqualToOp
  {
    VISKORES_EXEC
    bool operator()(const EdgeInterpolation& v1, const EdgeInterpolation& v2) const
    {
      return v1.Vertex1 == v2.Vertex1 && v1.Vertex2 == v2.Vertex2;
    }
  };
};

namespace internal
{

template <typename T>
VISKORES_EXEC_CONT T Scale(const T& val, viskores::Float64 scale)
{
  return static_cast<T>(scale * static_cast<viskores::Float64>(val));
}

template <typename T, viskores::IdComponent NumComponents>
VISKORES_EXEC_CONT viskores::Vec<T, NumComponents> Scale(const viskores::Vec<T, NumComponents>& val,
                                                         viskores::Float64 scale)
{
  return val * scale;
}

template <typename Device>
class ExecutionConnectivityExplicit
{
private:
  using UInt8Portal = typename viskores::cont::ArrayHandle<viskores::UInt8>::WritePortalType;
  using IdComponentPortal =
    typename viskores::cont::ArrayHandle<viskores::IdComponent>::WritePortalType;
  using IdPortal = typename viskores::cont::ArrayHandle<viskores::Id>::WritePortalType;

public:
  VISKORES_CONT
  ExecutionConnectivityExplicit() = default;

  VISKORES_CONT
  ExecutionConnectivityExplicit(viskores::cont::ArrayHandle<viskores::UInt8> shapes,
                                viskores::cont::ArrayHandle<viskores::IdComponent> numberOfIndices,
                                viskores::cont::ArrayHandle<viskores::Id> connectivity,
                                viskores::cont::ArrayHandle<viskores::Id> offsets,
                                ClipStats stats,
                                viskores::cont::Token& token)
    : Shapes(shapes.PrepareForOutput(stats.NumberOfCells, Device(), token))
    , NumberOfIndices(numberOfIndices.PrepareForOutput(stats.NumberOfCells, Device(), token))
    , Connectivity(connectivity.PrepareForOutput(stats.NumberOfIndices, Device(), token))
    , Offsets(offsets.PrepareForOutput(stats.NumberOfCells, Device(), token))
  {
  }

  VISKORES_EXEC
  void SetCellShape(viskores::Id cellIndex, viskores::UInt8 shape)
  {
    this->Shapes.Set(cellIndex, shape);
  }

  VISKORES_EXEC
  void SetNumberOfIndices(viskores::Id cellIndex, viskores::IdComponent numIndices)
  {
    this->NumberOfIndices.Set(cellIndex, numIndices);
  }

  VISKORES_EXEC
  void SetIndexOffset(viskores::Id cellIndex, viskores::Id indexOffset)
  {
    this->Offsets.Set(cellIndex, indexOffset);
  }

  VISKORES_EXEC
  void SetConnectivity(viskores::Id connectivityIndex, viskores::Id pointIndex)
  {
    this->Connectivity.Set(connectivityIndex, pointIndex);
  }

private:
  UInt8Portal Shapes;
  IdComponentPortal NumberOfIndices;
  IdPortal Connectivity;
  IdPortal Offsets;
};

class ConnectivityExplicit : viskores::cont::ExecutionObjectBase
{
public:
  VISKORES_CONT
  ConnectivityExplicit() = default;

  VISKORES_CONT
  ConnectivityExplicit(const viskores::cont::ArrayHandle<viskores::UInt8>& shapes,
                       const viskores::cont::ArrayHandle<viskores::IdComponent>& numberOfIndices,
                       const viskores::cont::ArrayHandle<viskores::Id>& connectivity,
                       const viskores::cont::ArrayHandle<viskores::Id>& offsets,
                       const ClipStats& stats)
    : Shapes(shapes)
    , NumberOfIndices(numberOfIndices)
    , Connectivity(connectivity)
    , Offsets(offsets)
    , Stats(stats)
  {
  }

  template <typename Device>
  VISKORES_CONT ExecutionConnectivityExplicit<Device> PrepareForExecution(
    Device,
    viskores::cont::Token& token) const
  {
    ExecutionConnectivityExplicit<Device> execConnectivity(
      this->Shapes, this->NumberOfIndices, this->Connectivity, this->Offsets, this->Stats, token);
    return execConnectivity;
  }

private:
  viskores::cont::ArrayHandle<viskores::UInt8> Shapes;
  viskores::cont::ArrayHandle<viskores::IdComponent> NumberOfIndices;
  viskores::cont::ArrayHandle<viskores::Id> Connectivity;
  viskores::cont::ArrayHandle<viskores::Id> Offsets;
  viskores::worklet::ClipStats Stats;
};


} // namespace internal

class Clip
{
  // Add support for invert
public:
  using TypeClipStats = viskores::List<ClipStats>;

  using TypeEdgeInterp = viskores::List<EdgeInterpolation>;

  class ComputeStats : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VISKORES_CONT
    ComputeStats(viskores::Float64 value, bool invert)
      : Value(value)
      , Invert(invert)
    {
    }

    using ControlSignature =
      void(CellSetIn, FieldInPoint, ExecObject clippingData, FieldOutCell, FieldOutCell);

    using ExecutionSignature = void(CellShape, PointCount, _2, _3, _4, _5);

    using InputDomain = _1;

    template <typename CellShapeTag, typename ScalarFieldVec, typename DeviceAdapter>
    VISKORES_EXEC void operator()(
      CellShapeTag shape,
      viskores::IdComponent pointCount,
      const ScalarFieldVec& scalars,
      const internal::ClipTables::DevicePortal<DeviceAdapter>& clippingData,
      ClipStats& clipStat,
      viskores::Id& clipDataIndex) const
    {
      (void)shape; // C4100 false positive workaround
      viskores::Id caseId = 0;
      for (viskores::IdComponent iter = pointCount - 1; iter >= 0; iter--)
      {
        if (!this->Invert && static_cast<viskores::Float64>(scalars[iter]) <= this->Value)
        {
          caseId++;
        }
        else if (this->Invert && static_cast<viskores::Float64>(scalars[iter]) >= this->Value)
        {
          caseId++;
        }
        if (iter > 0)
          caseId *= 2;
      }
      viskores::Id index = clippingData.GetCaseIndex(shape.Id, caseId);
      clipDataIndex = index;
      viskores::Id numberOfCells = clippingData.ValueAt(index++);
      clipStat.NumberOfCells = numberOfCells;
      for (viskores::IdComponent shapes = 0; shapes < numberOfCells; shapes++)
      {
        viskores::Id cellShape = clippingData.ValueAt(index++);
        viskores::Id numberOfIndices = clippingData.ValueAt(index++);
        if (cellShape == 0)
        {
          --clipStat.NumberOfCells;
          // Shape is 0, which is a case of interpolating new point within a cell
          // Gather stats for later operation.
          clipStat.NumberOfInCellPoints = 1;
          clipStat.NumberOfInCellInterpPoints = numberOfIndices;
          for (viskores::IdComponent points = 0; points < numberOfIndices; points++, index++)
          {
            //Find how many points need to be calculated using edge interpolation.
            viskores::Id element = clippingData.ValueAt(index);
            clipStat.NumberOfInCellEdgeIndices += (element < 100) ? 1 : 0;
          }
        }
        else
        {
          // Collect number of indices required for storing current shape
          clipStat.NumberOfIndices += numberOfIndices;
          // Collect number of new points
          for (viskores::IdComponent points = 0; points < numberOfIndices; points++, index++)
          {
            //Find how many points need to found using edge interpolation.
            viskores::Id element = clippingData.ValueAt(index);
            if (element == 255)
            {
              clipStat.NumberOfInCellIndices++;
            }
            else if (element < 100)
            {
              clipStat.NumberOfEdgeIndices++;
            }
          }
        }
      }
    }

  private:
    viskores::Float64 Value;
    bool Invert;
  };

  class GenerateCellSet : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VISKORES_CONT
    GenerateCellSet(viskores::Float64 value)
      : Value(value)
    {
    }

    using ControlSignature = void(CellSetIn,
                                  FieldInPoint,
                                  FieldInCell clipTableIndices,
                                  FieldInCell clipStats,
                                  ExecObject clipTables,
                                  ExecObject connectivityObject,
                                  WholeArrayOut pointsOnlyConnectivityIndices,
                                  WholeArrayOut edgePointReverseConnectivity,
                                  WholeArrayOut edgePointInterpolation,
                                  WholeArrayOut inCellReverseConnectivity,
                                  WholeArrayOut inCellEdgeReverseConnectivity,
                                  WholeArrayOut inCellEdgeInterpolation,
                                  WholeArrayOut inCellInterpolationKeys,
                                  WholeArrayOut inCellInterpolationInfo,
                                  WholeArrayOut cellMapOutputToInput);

    using ExecutionSignature = void(CellShape,
                                    WorkIndex,
                                    PointIndices,
                                    _2,
                                    _3,
                                    _4,
                                    _5,
                                    _6,
                                    _7,
                                    _8,
                                    _9,
                                    _10,
                                    _11,
                                    _12,
                                    _13,
                                    _14,
                                    _15);

    template <typename CellShapeTag,
              typename PointVecType,
              typename ScalarVecType,
              typename ConnectivityObject,
              typename IdArrayType,
              typename EdgeInterpolationPortalType,
              typename DeviceAdapter>
    VISKORES_EXEC void operator()(
      CellShapeTag shape,
      viskores::Id workIndex,
      const PointVecType& points,
      const ScalarVecType& scalars,
      viskores::Id clipDataIndex,
      const ClipStats& clipStats,
      const internal::ClipTables::DevicePortal<DeviceAdapter>& clippingData,
      ConnectivityObject& connectivityObject,
      IdArrayType& pointsOnlyConnectivityIndices,
      IdArrayType& edgePointReverseConnectivity,
      EdgeInterpolationPortalType& edgePointInterpolation,
      IdArrayType& inCellReverseConnectivity,
      IdArrayType& inCellEdgeReverseConnectivity,
      EdgeInterpolationPortalType& inCellEdgeInterpolation,
      IdArrayType& inCellInterpolationKeys,
      IdArrayType& inCellInterpolationInfo,
      IdArrayType& cellMapOutputToInput) const
    {
      (void)shape;
      viskores::Id clipIndex = clipDataIndex;
      // Start index for the cells of this case.
      viskores::Id cellIndex = clipStats.NumberOfCells;
      // Start index to store connevtivity of this case.
      viskores::Id connectivityIndex = clipStats.NumberOfIndices;
      // Start indices for reverse mapping into connectivity for this case.
      viskores::Id edgeIndex = clipStats.NumberOfEdgeIndices;
      viskores::Id inCellIndex = clipStats.NumberOfInCellIndices;
      viskores::Id inCellPoints = clipStats.NumberOfInCellPoints;
      // Start Indices to keep track of interpolation points for new cell.
      viskores::Id inCellInterpPointIndex = clipStats.NumberOfInCellInterpPoints;
      viskores::Id inCellEdgeInterpIndex = clipStats.NumberOfInCellEdgeIndices;
      // Start index of connectivityPointsOnly
      viskores::Id pointsOnlyConnectivityIndicesIndex = connectivityIndex - edgeIndex - inCellIndex;

      // Iterate over the shapes for the current cell and begin to fill connectivity.
      viskores::Id numberOfCells = clippingData.ValueAt(clipIndex++);
      for (viskores::Id cell = 0; cell < numberOfCells; ++cell)
      {
        viskores::UInt8 cellShape = clippingData.ValueAt(clipIndex++);
        viskores::IdComponent numberOfPoints = clippingData.ValueAt(clipIndex++);
        if (cellShape == 0)
        {
          // Case for a new cell point.

          // 1. Output the input cell id for which we need to generate new point.
          // 2. Output number of points used for interpolation.
          // 3. If vertex
          //    - Add vertex to connectivity interpolation information.
          // 4. If edge
          //    - Add edge interpolation information for new points.
          //    - Reverse connectivity map for new points.
          // Make an array which has all the elements that need to be used
          // for interpolation.
          for (viskores::IdComponent point = 0; point < numberOfPoints;
               point++, inCellInterpPointIndex++, clipIndex++)
          {
            viskores::IdComponent entry =
              static_cast<viskores::IdComponent>(clippingData.ValueAt(clipIndex));
            inCellInterpolationKeys.Set(inCellInterpPointIndex, workIndex);
            if (entry >= 100)
            {
              inCellInterpolationInfo.Set(inCellInterpPointIndex, points[entry - 100]);
            }
            else
            {
              internal::ClipTables::EdgeVec edge = clippingData.GetEdge(shape.Id, entry);
              VISKORES_ASSERT(edge[0] != 255);
              VISKORES_ASSERT(edge[1] != 255);
              EdgeInterpolation ei;
              ei.Vertex1 = points[edge[0]];
              ei.Vertex2 = points[edge[1]];
              // For consistency purposes keep the points ordered.
              if (ei.Vertex1 > ei.Vertex2)
              {
                this->swap(ei.Vertex1, ei.Vertex2);
                this->swap(edge[0], edge[1]);
              }
              ei.Weight = (static_cast<viskores::Float64>(scalars[edge[0]]) - this->Value) /
                static_cast<viskores::Float64>(scalars[edge[1]] - scalars[edge[0]]);

              inCellEdgeReverseConnectivity.Set(inCellEdgeInterpIndex, inCellInterpPointIndex);
              inCellEdgeInterpolation.Set(inCellEdgeInterpIndex, ei);
              inCellEdgeInterpIndex++;
            }
          }
        }
        else
        {
          // Just a normal cell, generate edge representations,

          // 1. Add cell type to connectivity information.
          // 2. If vertex
          //    - Add vertex to connectivity information.
          // 3. If edge point
          //    - Add edge to edge points
          //    - Add edge point index to edge point reverse connectivity.
          // 4. If cell point
          //    - Add cell point index to connectivity
          //      (as there is only one cell point per required cell)
          // 5. Store input cell index against current cell for mapping cell data.
          connectivityObject.SetCellShape(cellIndex, cellShape);
          connectivityObject.SetNumberOfIndices(cellIndex, numberOfPoints);
          connectivityObject.SetIndexOffset(cellIndex, connectivityIndex);
          for (viskores::IdComponent point = 0; point < numberOfPoints; point++, clipIndex++)
          {
            viskores::IdComponent entry =
              static_cast<viskores::IdComponent>(clippingData.ValueAt(clipIndex));
            if (entry == 255) // case of cell point interpolation
            {
              // Add index of the corresponding cell point.
              inCellReverseConnectivity.Set(inCellIndex++, connectivityIndex);
              connectivityObject.SetConnectivity(connectivityIndex, inCellPoints);
              connectivityIndex++;
            }
            else if (entry >= 100) // existing vertex
            {
              pointsOnlyConnectivityIndices.Set(pointsOnlyConnectivityIndicesIndex++,
                                                connectivityIndex);
              connectivityObject.SetConnectivity(connectivityIndex++, points[entry - 100]);
            }
            else // case of a new edge point
            {
              internal::ClipTables::EdgeVec edge = clippingData.GetEdge(shape.Id, entry);
              VISKORES_ASSERT(edge[0] != 255);
              VISKORES_ASSERT(edge[1] != 255);
              EdgeInterpolation ei;
              ei.Vertex1 = points[edge[0]];
              ei.Vertex2 = points[edge[1]];
              // For consistency purposes keep the points ordered.
              if (ei.Vertex1 > ei.Vertex2)
              {
                this->swap(ei.Vertex1, ei.Vertex2);
                this->swap(edge[0], edge[1]);
              }
              ei.Weight = (static_cast<viskores::Float64>(scalars[edge[0]]) - this->Value) /
                static_cast<viskores::Float64>(scalars[edge[1]] - scalars[edge[0]]);
              //Add to set of new edge points
              //Add reverse connectivity;
              edgePointReverseConnectivity.Set(edgeIndex, connectivityIndex++);
              edgePointInterpolation.Set(edgeIndex, ei);
              edgeIndex++;
            }
          }
          cellMapOutputToInput.Set(cellIndex, workIndex);
          ++cellIndex;
        }
      }
    }

    template <typename T>
    VISKORES_EXEC void swap(T& v1, T& v2) const
    {
      T temp = v1;
      v1 = v2;
      v2 = temp;
    }

  private:
    viskores::Float64 Value;
  };

  class ScatterEdgeConnectivity : public viskores::worklet::WorkletMapField
  {
  public:
    VISKORES_CONT
    ScatterEdgeConnectivity(viskores::Id edgePointOffset)
      : EdgePointOffset(edgePointOffset)
    {
    }

    using ControlSignature = void(FieldIn sourceValue,
                                  FieldIn destinationIndices,
                                  WholeArrayOut destinationData);

    using ExecutionSignature = void(_1, _2, _3);

    using InputDomain = _1;

    template <typename ConnectivityDataType>
    VISKORES_EXEC void operator()(viskores::Id sourceValue,
                                  viskores::Id destinationIndex,
                                  ConnectivityDataType& destinationData) const
    {
      destinationData.Set(destinationIndex, (sourceValue + EdgePointOffset));
    }

  private:
    viskores::Id EdgePointOffset;
  };

  class ScatterInCellConnectivity : public viskores::worklet::WorkletMapField
  {
  public:
    VISKORES_CONT
    ScatterInCellConnectivity(viskores::Id inCellPointOffset)
      : InCellPointOffset(inCellPointOffset)
    {
    }

    using ControlSignature = void(FieldIn destinationIndices, WholeArrayOut destinationData);

    using ExecutionSignature = void(_1, _2);

    using InputDomain = _1;

    template <typename ConnectivityDataType>
    VISKORES_EXEC void operator()(viskores::Id destinationIndex,
                                  ConnectivityDataType& destinationData) const
    {
      auto sourceValue = destinationData.Get(destinationIndex);
      destinationData.Set(destinationIndex, (sourceValue + InCellPointOffset));
    }

  private:
    viskores::Id InCellPointOffset;
  };

  Clip()
    : ClipTablesInstance()
    , EdgePointsInterpolation()
    , InCellInterpolationKeys()
    , InCellInterpolationInfo()
    , CellMapOutputToInput()
    , EdgePointsOffset()
    , InCellPointsOffset()
  {
  }

  template <typename CellSetType, typename ScalarsArrayHandle>
  viskores::cont::CellSetExplicit<> Run(const CellSetType& cellSet,
                                        const ScalarsArrayHandle& scalars,
                                        viskores::Float64 value,
                                        bool invert)
  {
    viskores::cont::Invoker invoke;

    // Create the required output fields.
    viskores::cont::ArrayHandle<ClipStats> clipStats;
    viskores::cont::ArrayHandle<viskores::Id> clipTableIndices;

    //Send this CellSet to process
    ComputeStats statsWorklet(value, invert);
    invoke(statsWorklet, cellSet, scalars, this->ClipTablesInstance, clipStats, clipTableIndices);

    ClipStats zero;
    viskores::cont::ArrayHandle<ClipStats> cellSetStats;
    ClipStats total =
      viskores::cont::Algorithm::ScanExclusive(clipStats, cellSetStats, ClipStats::SumOp(), zero);
    clipStats.ReleaseResources();

    viskores::cont::ArrayHandle<viskores::UInt8> shapes;
    viskores::cont::ArrayHandle<viskores::IdComponent> numberOfIndices;
    viskores::cont::ArrayHandle<viskores::Id> connectivity;
    viskores::cont::ArrayHandle<viskores::Id> offsets;
    internal::ConnectivityExplicit connectivityObject(
      shapes, numberOfIndices, connectivity, offsets, total);

    //Begin Process of Constructing the new CellSet.
    viskores::cont::ArrayHandle<viskores::Id> pointsOnlyConnectivityIndices;
    pointsOnlyConnectivityIndices.Allocate(total.NumberOfIndices - total.NumberOfEdgeIndices -
                                           total.NumberOfInCellIndices);

    viskores::cont::ArrayHandle<viskores::Id> edgePointReverseConnectivity;
    edgePointReverseConnectivity.Allocate(total.NumberOfEdgeIndices);
    viskores::cont::ArrayHandle<EdgeInterpolation> edgeInterpolation;
    edgeInterpolation.Allocate(total.NumberOfEdgeIndices);

    viskores::cont::ArrayHandle<viskores::Id> cellPointReverseConnectivity;
    cellPointReverseConnectivity.Allocate(total.NumberOfInCellIndices);
    viskores::cont::ArrayHandle<viskores::Id> cellPointEdgeReverseConnectivity;
    cellPointEdgeReverseConnectivity.Allocate(total.NumberOfInCellEdgeIndices);
    viskores::cont::ArrayHandle<EdgeInterpolation> cellPointEdgeInterpolation;
    cellPointEdgeInterpolation.Allocate(total.NumberOfInCellEdgeIndices);

    this->InCellInterpolationKeys.Allocate(total.NumberOfInCellInterpPoints);
    this->InCellInterpolationInfo.Allocate(total.NumberOfInCellInterpPoints);
    this->CellMapOutputToInput.Allocate(total.NumberOfCells);

    //Send this CellSet to process
    GenerateCellSet cellSetWorklet(value);
    invoke(cellSetWorklet,
           cellSet,
           scalars,
           clipTableIndices,
           cellSetStats,
           this->ClipTablesInstance,
           connectivityObject,
           pointsOnlyConnectivityIndices,
           edgePointReverseConnectivity,
           edgeInterpolation,
           cellPointReverseConnectivity,
           cellPointEdgeReverseConnectivity,
           cellPointEdgeInterpolation,
           this->InCellInterpolationKeys,
           this->InCellInterpolationInfo,
           this->CellMapOutputToInput);
    this->InterpolationKeysBuilt = false;

    clipTableIndices.ReleaseResources();
    cellSetStats.ReleaseResources();

    // extract only the used points from the input
    {
      viskores::cont::ArrayHandle<viskores::IdComponent> pointMask;
      pointMask.AllocateAndFill(scalars.GetNumberOfValues(), 0);

      auto pointsOnlyConnectivity =
        viskores::cont::make_ArrayHandlePermutation(pointsOnlyConnectivityIndices, connectivity);

      invoke(viskores::worklet::RemoveUnusedPoints::GeneratePointMask{},
             pointsOnlyConnectivity,
             pointMask);

      viskores::worklet::ScatterCounting scatter(pointMask, true);
      auto pointMapInputToOutput = scatter.GetInputToOutputMap();
      this->PointMapOutputToInput = scatter.GetOutputToInputMap();
      pointMask.ReleaseResources();

      invoke(viskores::worklet::RemoveUnusedPoints::TransformPointIndices{},
             pointsOnlyConnectivity,
             pointMapInputToOutput,
             pointsOnlyConnectivity);

      pointsOnlyConnectivityIndices.ReleaseResources();

      // We want to find the entries in `InCellInterpolationInfo` that point to exisiting points.
      // `cellPointEdgeReverseConnectivity` map to entries that point to edges.
      viskores::cont::ArrayHandle<viskores::UInt8> stencil;
      stencil.AllocateAndFill(this->InCellInterpolationInfo.GetNumberOfValues(), 1);
      auto edgeOnlyStencilEntries =
        viskores::cont::make_ArrayHandlePermutation(cellPointEdgeReverseConnectivity, stencil);
      viskores::cont::Algorithm::Fill(edgeOnlyStencilEntries, viskores::UInt8{});
      viskores::cont::ArrayHandle<viskores::Id> idxsToPoints;
      viskores::cont::Algorithm::CopyIf(
        viskores::cont::ArrayHandleIndex(this->InCellInterpolationInfo.GetNumberOfValues()),
        stencil,
        idxsToPoints);
      stencil.ReleaseResources();

      // Remap the point indices in `InCellInterpolationInfo`, to the used-only point indices
      // computed above.
      // This only works if the points needed for interpolating centroids are included in the
      // `connectivity` array. This has been verified to be true for all cases in the clip tables.
      auto inCellInterpolationInfoPointsOnly =
        viskores::cont::make_ArrayHandlePermutation(idxsToPoints, this->InCellInterpolationInfo);
      invoke(viskores::worklet::RemoveUnusedPoints::TransformPointIndices{},
             inCellInterpolationInfoPointsOnly,
             pointMapInputToOutput,
             inCellInterpolationInfoPointsOnly);
    }

    // Get unique EdgeInterpolation : unique edge points.
    // LowerBound for edgeInterpolation : get index into new edge points array.
    // LowerBound for cellPointEdgeInterpolation : get index into new edge points array.
    viskores::cont::Algorithm::SortByKey(
      edgeInterpolation, edgePointReverseConnectivity, EdgeInterpolation::LessThanOp());
    viskores::cont::Algorithm::Copy(edgeInterpolation, this->EdgePointsInterpolation);
    viskores::cont::Algorithm::Unique(this->EdgePointsInterpolation,
                                      EdgeInterpolation::EqualToOp());

    viskores::cont::ArrayHandle<viskores::Id> edgeInterpolationIndexToUnique;
    viskores::cont::Algorithm::LowerBounds(this->EdgePointsInterpolation,
                                           edgeInterpolation,
                                           edgeInterpolationIndexToUnique,
                                           EdgeInterpolation::LessThanOp());
    edgeInterpolation.ReleaseResources();

    // This only works if the edges in `cellPointEdgeInterpolation` also exist in
    // `EdgePointsInterpolation`. This has been verified to be true for all cases in the clip
    // tables.
    viskores::cont::ArrayHandle<viskores::Id> cellInterpolationIndexToUnique;
    viskores::cont::Algorithm::LowerBounds(this->EdgePointsInterpolation,
                                           cellPointEdgeInterpolation,
                                           cellInterpolationIndexToUnique,
                                           EdgeInterpolation::LessThanOp());
    cellPointEdgeInterpolation.ReleaseResources();

    this->EdgePointsOffset = this->PointMapOutputToInput.GetNumberOfValues();
    this->InCellPointsOffset =
      this->EdgePointsOffset + this->EdgePointsInterpolation.GetNumberOfValues();

    // Scatter these values into the connectivity array,
    // scatter indices are given in reverse connectivity.
    ScatterEdgeConnectivity scatterEdgePointConnectivity(this->EdgePointsOffset);
    invoke(scatterEdgePointConnectivity,
           edgeInterpolationIndexToUnique,
           edgePointReverseConnectivity,
           connectivity);
    invoke(scatterEdgePointConnectivity,
           cellInterpolationIndexToUnique,
           cellPointEdgeReverseConnectivity,
           this->InCellInterpolationInfo);

    // Add offset in connectivity of all new in-cell points.
    ScatterInCellConnectivity scatterInCellPointConnectivity(this->InCellPointsOffset);
    invoke(scatterInCellPointConnectivity, cellPointReverseConnectivity, connectivity);

    viskores::cont::CellSetExplicit<> output;
    viskores::Id numberOfPoints = this->PointMapOutputToInput.GetNumberOfValues() +
      this->EdgePointsInterpolation.GetNumberOfValues() + total.NumberOfInCellPoints;

    viskores::cont::ConvertNumComponentsToOffsets(numberOfIndices, offsets);

    output.Fill(numberOfPoints, shapes, connectivity, offsets);
    return output;
  }

  template <typename CellSetType, typename ImplicitFunction>
  class ClipWithImplicitFunction
  {
  public:
    VISKORES_CONT
    ClipWithImplicitFunction(Clip* clipper,
                             const CellSetType& cellSet,
                             const ImplicitFunction& function,
                             viskores::Float64 offset,
                             bool invert,
                             viskores::cont::CellSetExplicit<>* result)
      : Clipper(clipper)
      , CellSet(&cellSet)
      , Function(function)
      , Offset(offset)
      , Invert(invert)
      , Result(result)
    {
    }

    template <typename ArrayHandleType>
    VISKORES_CONT void operator()(const ArrayHandleType& handle) const
    {
      // Evaluate the implicit function on the input coordinates using
      // ArrayHandleTransform
      viskores::cont::ArrayHandleTransform<ArrayHandleType,
                                           viskores::ImplicitFunctionValueFunctor<ImplicitFunction>>
        clipScalars(handle, this->Function);

      // Clip at locations where the implicit function evaluates to `Offset`
      *this->Result = this->Clipper->Run(*this->CellSet, clipScalars, this->Offset, this->Invert);
    }

  private:
    Clip* Clipper;
    const CellSetType* CellSet;
    ImplicitFunction Function;
    viskores::Float64 Offset;
    bool Invert;
    viskores::cont::CellSetExplicit<>* Result;
  };

  template <typename CellSetType, typename ImplicitFunction>
  viskores::cont::CellSetExplicit<> Run(const CellSetType& cellSet,
                                        const ImplicitFunction& clipFunction,
                                        viskores::Float64 offset,
                                        const viskores::cont::CoordinateSystem& coords,
                                        bool invert)
  {
    viskores::cont::CellSetExplicit<> output;

    ClipWithImplicitFunction<CellSetType, ImplicitFunction> clip(
      this, cellSet, clipFunction, offset, invert, &output);

    CastAndCall(coords, clip);
    return output;
  }

  template <typename CellSetType, typename ImplicitFunction>
  viskores::cont::CellSetExplicit<> Run(const CellSetType& cellSet,
                                        const ImplicitFunction& clipFunction,
                                        const viskores::cont::CoordinateSystem& coords,
                                        bool invert)
  {
    return this->Run(cellSet, clipFunction, 0.0, coords, invert);
  }

  struct PerformEdgeInterpolations : public viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn edgeInterpolations,
                                  WholeArrayIn originalField,
                                  FieldOut outputField);
    using ExecutionSignature = void(_1, _2, _3);

    template <typename FieldPortal, typename T>
    VISKORES_EXEC void operator()(const EdgeInterpolation& edgeInterp,
                                  const FieldPortal& originalField,
                                  T& output) const
    {
      T v1 = originalField.Get(edgeInterp.Vertex1);
      T v2 = originalField.Get(edgeInterp.Vertex2);

      // Interpolate per-vertex because some vec-like objects do not allow intermediate variables
      using VTraits = viskores::VecTraits<T>;
      using CType = typename VTraits::ComponentType;
      VISKORES_ASSERT(VTraits::GetNumberOfComponents(v1) == VTraits::GetNumberOfComponents(output));
      VISKORES_ASSERT(VTraits::GetNumberOfComponents(v2) == VTraits::GetNumberOfComponents(output));
      for (viskores::IdComponent component = 0; component < VTraits::GetNumberOfComponents(output);
           ++component)
      {
        CType c1 = VTraits::GetComponent(v1, component);
        CType c2 = VTraits::GetComponent(v2, component);
        CType o = static_cast<CType>(((c1 - c2) * edgeInterp.Weight) + c1);
        VTraits::SetComponent(output, component, o);
      }
    }
  };

  struct PerformInCellInterpolations : public viskores::worklet::WorkletReduceByKey
  {
    using ControlSignature = void(KeysIn keys, ValuesIn toReduce, ReducedValuesOut centroids);
    using ExecutionSignature = void(_2, _3);

    template <typename MappedValueVecType, typename MappedValueType>
    VISKORES_EXEC void operator()(const MappedValueVecType& toReduce,
                                  MappedValueType& centroid) const
    {
      const viskores::IdComponent numValues = toReduce.GetNumberOfComponents();

      // Interpolate per-vertex because some vec-like objects do not allow intermediate variables
      using VTraits = viskores::VecTraits<MappedValueType>;
      using CType = typename VTraits::ComponentType;
      for (viskores::IdComponent component = 0;
           component < VTraits::GetNumberOfComponents(centroid);
           ++component)
      {
        CType sum = VTraits::GetComponent(toReduce[0], component);
        for (viskores::IdComponent reduceI = 1; reduceI < numValues; ++reduceI)
        {
          // static_cast is for when MappedValueType is a small int that gets promoted to int32.
          sum = static_cast<CType>(sum + VTraits::GetComponent(toReduce[reduceI], component));
        }
        VTraits::SetComponent(centroid, component, static_cast<CType>(sum / numValues));
      }
    }
  };

  template <typename InputType, typename OutputType>
  void ProcessPointField(const InputType& input, OutputType& output)
  {
    if (!this->InterpolationKeysBuilt)
    {
      this->InterpolationKeys.BuildArrays(this->InCellInterpolationKeys, KeysSortType::Unstable);
    }

    viskores::Id numberOfVertexPoints = this->PointMapOutputToInput.GetNumberOfValues();
    viskores::Id numberOfEdgePoints = this->EdgePointsInterpolation.GetNumberOfValues();
    viskores::Id numberOfInCellPoints = this->InterpolationKeys.GetUniqueKeys().GetNumberOfValues();

    output.Allocate(numberOfVertexPoints + numberOfEdgePoints + numberOfInCellPoints);

    // Copy over the original values that are still part of the output.
    viskores::cont::Algorithm::CopySubRange(
      viskores::cont::make_ArrayHandlePermutation(this->PointMapOutputToInput, input),
      0,
      numberOfVertexPoints,
      output);

    // Interpolate all new points that lie on edges of the input mesh.
    viskores::cont::Invoker invoke;
    invoke(PerformEdgeInterpolations{},
           this->EdgePointsInterpolation,
           input,
           viskores::cont::make_ArrayHandleView(output, numberOfVertexPoints, numberOfEdgePoints));

    // Perform a gather on the output to get all the required values for calculation of centroids
    // using the interpolation info array.
    auto toReduceValues =
      viskores::cont::make_ArrayHandlePermutation(this->InCellInterpolationInfo, output);
    invoke(PerformInCellInterpolations{},
           this->InterpolationKeys,
           toReduceValues,
           viskores::cont::make_ArrayHandleView(
             output, numberOfVertexPoints + numberOfEdgePoints, numberOfInCellPoints));
  }

  viskores::cont::ArrayHandle<viskores::Id> GetCellMapOutputToInput() const
  {
    return this->CellMapOutputToInput;
  }

private:
  internal::ClipTables ClipTablesInstance;
  viskores::cont::ArrayHandle<EdgeInterpolation> EdgePointsInterpolation;
  viskores::cont::ArrayHandle<viskores::Id> InCellInterpolationKeys;
  viskores::cont::ArrayHandle<viskores::Id> InCellInterpolationInfo;
  viskores::cont::ArrayHandle<viskores::Id> CellMapOutputToInput;
  viskores::cont::ArrayHandle<viskores::Id> PointMapOutputToInput;
  viskores::Id EdgePointsOffset;
  viskores::Id InCellPointsOffset;
  viskores::worklet::Keys<viskores::Id> InterpolationKeys;
  bool InterpolationKeysBuilt = false;
};
}
} // namespace viskores::worklet

#if defined(THRUST_SCAN_WORKAROUND)
namespace thrust
{
namespace detail
{

// causes a different code path which does not have the bug
template <>
struct is_integral<viskores::worklet::ClipStats> : public true_type
{
};
}
} // namespace thrust::detail
#endif

#endif // viskores_m_worklet_Clip_h
