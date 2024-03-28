//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_m_worklet_Clip_h
#define vtkm_m_worklet_Clip_h

#include <vtkm/filter/clean_grid/worklet/RemoveUnusedPoints.h>
#include <vtkm/filter/contour/worklet/clip/ClipTables.h>
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/ImplicitFunction.h>

#include <utility>
#include <vtkm/exec/FunctorBase.h>

#if defined(THRUST_MAJOR_VERSION) && THRUST_MAJOR_VERSION == 1 && THRUST_MINOR_VERSION == 8 && \
  THRUST_SUBMINOR_VERSION < 3
// Workaround a bug in thrust 1.8.0 - 1.8.2 scan implementations which produces
// wrong results
#include <vtkm/exec/cuda/internal/ThrustPatches.h>
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/detail/type_traits.h>
VTKM_THIRDPARTY_POST_INCLUDE
#define THRUST_SCAN_WORKAROUND
#endif

namespace vtkm
{
namespace worklet
{
struct ClipStats
{
  vtkm::Id NumberOfCells = 0;
  vtkm::Id NumberOfIndices = 0;
  vtkm::Id NumberOfEdgeIndices = 0;

  // Stats for interpolating new points within cell.
  vtkm::Id NumberOfInCellPoints = 0;
  vtkm::Id NumberOfInCellIndices = 0;
  vtkm::Id NumberOfInCellInterpPoints = 0;
  vtkm::Id NumberOfInCellEdgeIndices = 0;

  struct SumOp
  {
    VTKM_EXEC_CONT
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
  vtkm::Id Vertex1 = -1;
  vtkm::Id Vertex2 = -1;
  vtkm::Float64 Weight = 0;

  struct LessThanOp
  {
    VTKM_EXEC
    bool operator()(const EdgeInterpolation& v1, const EdgeInterpolation& v2) const
    {
      return (v1.Vertex1 < v2.Vertex1) || (v1.Vertex1 == v2.Vertex1 && v1.Vertex2 < v2.Vertex2);
    }
  };

  struct EqualToOp
  {
    VTKM_EXEC
    bool operator()(const EdgeInterpolation& v1, const EdgeInterpolation& v2) const
    {
      return v1.Vertex1 == v2.Vertex1 && v1.Vertex2 == v2.Vertex2;
    }
  };
};

namespace internal
{

template <typename T>
VTKM_EXEC_CONT T Scale(const T& val, vtkm::Float64 scale)
{
  return static_cast<T>(scale * static_cast<vtkm::Float64>(val));
}

template <typename T, vtkm::IdComponent NumComponents>
VTKM_EXEC_CONT vtkm::Vec<T, NumComponents> Scale(const vtkm::Vec<T, NumComponents>& val,
                                                 vtkm::Float64 scale)
{
  return val * scale;
}

template <typename Device>
class ExecutionConnectivityExplicit
{
private:
  using UInt8Portal = typename vtkm::cont::ArrayHandle<vtkm::UInt8>::WritePortalType;
  using IdComponentPortal = typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::WritePortalType;
  using IdPortal = typename vtkm::cont::ArrayHandle<vtkm::Id>::WritePortalType;

public:
  VTKM_CONT
  ExecutionConnectivityExplicit() = default;

  VTKM_CONT
  ExecutionConnectivityExplicit(vtkm::cont::ArrayHandle<vtkm::UInt8> shapes,
                                vtkm::cont::ArrayHandle<vtkm::IdComponent> numberOfIndices,
                                vtkm::cont::ArrayHandle<vtkm::Id> connectivity,
                                vtkm::cont::ArrayHandle<vtkm::Id> offsets,
                                ClipStats stats,
                                vtkm::cont::Token& token)
    : Shapes(shapes.PrepareForOutput(stats.NumberOfCells, Device(), token))
    , NumberOfIndices(numberOfIndices.PrepareForOutput(stats.NumberOfCells, Device(), token))
    , Connectivity(connectivity.PrepareForOutput(stats.NumberOfIndices, Device(), token))
    , Offsets(offsets.PrepareForOutput(stats.NumberOfCells, Device(), token))
  {
  }

  VTKM_EXEC
  void SetCellShape(vtkm::Id cellIndex, vtkm::UInt8 shape) { this->Shapes.Set(cellIndex, shape); }

  VTKM_EXEC
  void SetNumberOfIndices(vtkm::Id cellIndex, vtkm::IdComponent numIndices)
  {
    this->NumberOfIndices.Set(cellIndex, numIndices);
  }

  VTKM_EXEC
  void SetIndexOffset(vtkm::Id cellIndex, vtkm::Id indexOffset)
  {
    this->Offsets.Set(cellIndex, indexOffset);
  }

  VTKM_EXEC
  void SetConnectivity(vtkm::Id connectivityIndex, vtkm::Id pointIndex)
  {
    this->Connectivity.Set(connectivityIndex, pointIndex);
  }

private:
  UInt8Portal Shapes;
  IdComponentPortal NumberOfIndices;
  IdPortal Connectivity;
  IdPortal Offsets;
};

class ConnectivityExplicit : vtkm::cont::ExecutionObjectBase
{
public:
  VTKM_CONT
  ConnectivityExplicit() = default;

  VTKM_CONT
  ConnectivityExplicit(const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
                       const vtkm::cont::ArrayHandle<vtkm::IdComponent>& numberOfIndices,
                       const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
                       const vtkm::cont::ArrayHandle<vtkm::Id>& offsets,
                       const ClipStats& stats)
    : Shapes(shapes)
    , NumberOfIndices(numberOfIndices)
    , Connectivity(connectivity)
    , Offsets(offsets)
    , Stats(stats)
  {
  }

  template <typename Device>
  VTKM_CONT ExecutionConnectivityExplicit<Device> PrepareForExecution(
    Device,
    vtkm::cont::Token& token) const
  {
    ExecutionConnectivityExplicit<Device> execConnectivity(
      this->Shapes, this->NumberOfIndices, this->Connectivity, this->Offsets, this->Stats, token);
    return execConnectivity;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::UInt8> Shapes;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumberOfIndices;
  vtkm::cont::ArrayHandle<vtkm::Id> Connectivity;
  vtkm::cont::ArrayHandle<vtkm::Id> Offsets;
  vtkm::worklet::ClipStats Stats;
};


} // namespace internal

class Clip
{
  // Add support for invert
public:
  using TypeClipStats = vtkm::List<ClipStats>;

  using TypeEdgeInterp = vtkm::List<EdgeInterpolation>;

  class ComputeStats : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VTKM_CONT
    ComputeStats(vtkm::Float64 value, bool invert)
      : Value(value)
      , Invert(invert)
    {
    }

    using ControlSignature =
      void(CellSetIn, FieldInPoint, ExecObject clippingData, FieldOutCell, FieldOutCell);

    using ExecutionSignature = void(CellShape, PointCount, _2, _3, _4, _5);

    using InputDomain = _1;

    template <typename CellShapeTag, typename ScalarFieldVec, typename DeviceAdapter>
    VTKM_EXEC void operator()(CellShapeTag shape,
                              vtkm::IdComponent pointCount,
                              const ScalarFieldVec& scalars,
                              const internal::ClipTables::DevicePortal<DeviceAdapter>& clippingData,
                              ClipStats& clipStat,
                              vtkm::Id& clipDataIndex) const
    {
      (void)shape; // C4100 false positive workaround
      vtkm::Id caseId = 0;
      for (vtkm::IdComponent iter = pointCount - 1; iter >= 0; iter--)
      {
        if (!this->Invert && static_cast<vtkm::Float64>(scalars[iter]) <= this->Value)
        {
          caseId++;
        }
        else if (this->Invert && static_cast<vtkm::Float64>(scalars[iter]) >= this->Value)
        {
          caseId++;
        }
        if (iter > 0)
          caseId *= 2;
      }
      vtkm::Id index = clippingData.GetCaseIndex(shape.Id, caseId);
      clipDataIndex = index;
      vtkm::Id numberOfCells = clippingData.ValueAt(index++);
      clipStat.NumberOfCells = numberOfCells;
      for (vtkm::IdComponent shapes = 0; shapes < numberOfCells; shapes++)
      {
        vtkm::Id cellShape = clippingData.ValueAt(index++);
        vtkm::Id numberOfIndices = clippingData.ValueAt(index++);
        if (cellShape == 0)
        {
          --clipStat.NumberOfCells;
          // Shape is 0, which is a case of interpolating new point within a cell
          // Gather stats for later operation.
          clipStat.NumberOfInCellPoints = 1;
          clipStat.NumberOfInCellInterpPoints = numberOfIndices;
          for (vtkm::IdComponent points = 0; points < numberOfIndices; points++, index++)
          {
            //Find how many points need to be calculated using edge interpolation.
            vtkm::Id element = clippingData.ValueAt(index);
            clipStat.NumberOfInCellEdgeIndices += (element < 100) ? 1 : 0;
          }
        }
        else
        {
          // Collect number of indices required for storing current shape
          clipStat.NumberOfIndices += numberOfIndices;
          // Collect number of new points
          for (vtkm::IdComponent points = 0; points < numberOfIndices; points++, index++)
          {
            //Find how many points need to found using edge interpolation.
            vtkm::Id element = clippingData.ValueAt(index);
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
    vtkm::Float64 Value;
    bool Invert;
  };

  class GenerateCellSet : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VTKM_CONT
    GenerateCellSet(vtkm::Float64 value)
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
    VTKM_EXEC void operator()(CellShapeTag shape,
                              vtkm::Id workIndex,
                              const PointVecType& points,
                              const ScalarVecType& scalars,
                              vtkm::Id clipDataIndex,
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
      vtkm::Id clipIndex = clipDataIndex;
      // Start index for the cells of this case.
      vtkm::Id cellIndex = clipStats.NumberOfCells;
      // Start index to store connevtivity of this case.
      vtkm::Id connectivityIndex = clipStats.NumberOfIndices;
      // Start indices for reverse mapping into connectivity for this case.
      vtkm::Id edgeIndex = clipStats.NumberOfEdgeIndices;
      vtkm::Id inCellIndex = clipStats.NumberOfInCellIndices;
      vtkm::Id inCellPoints = clipStats.NumberOfInCellPoints;
      // Start Indices to keep track of interpolation points for new cell.
      vtkm::Id inCellInterpPointIndex = clipStats.NumberOfInCellInterpPoints;
      vtkm::Id inCellEdgeInterpIndex = clipStats.NumberOfInCellEdgeIndices;
      // Start index of connectivityPointsOnly
      vtkm::Id pointsOnlyConnectivityIndicesIndex = connectivityIndex - edgeIndex - inCellIndex;

      // Iterate over the shapes for the current cell and begin to fill connectivity.
      vtkm::Id numberOfCells = clippingData.ValueAt(clipIndex++);
      for (vtkm::Id cell = 0; cell < numberOfCells; ++cell)
      {
        vtkm::UInt8 cellShape = clippingData.ValueAt(clipIndex++);
        vtkm::IdComponent numberOfPoints = clippingData.ValueAt(clipIndex++);
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
          for (vtkm::IdComponent point = 0; point < numberOfPoints;
               point++, inCellInterpPointIndex++, clipIndex++)
          {
            vtkm::IdComponent entry =
              static_cast<vtkm::IdComponent>(clippingData.ValueAt(clipIndex));
            inCellInterpolationKeys.Set(inCellInterpPointIndex, workIndex);
            if (entry >= 100)
            {
              inCellInterpolationInfo.Set(inCellInterpPointIndex, points[entry - 100]);
            }
            else
            {
              internal::ClipTables::EdgeVec edge = clippingData.GetEdge(shape.Id, entry);
              VTKM_ASSERT(edge[0] != 255);
              VTKM_ASSERT(edge[1] != 255);
              EdgeInterpolation ei;
              ei.Vertex1 = points[edge[0]];
              ei.Vertex2 = points[edge[1]];
              // For consistency purposes keep the points ordered.
              if (ei.Vertex1 > ei.Vertex2)
              {
                this->swap(ei.Vertex1, ei.Vertex2);
                this->swap(edge[0], edge[1]);
              }
              ei.Weight = (static_cast<vtkm::Float64>(scalars[edge[0]]) - this->Value) /
                static_cast<vtkm::Float64>(scalars[edge[1]] - scalars[edge[0]]);

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
          for (vtkm::IdComponent point = 0; point < numberOfPoints; point++, clipIndex++)
          {
            vtkm::IdComponent entry =
              static_cast<vtkm::IdComponent>(clippingData.ValueAt(clipIndex));
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
              VTKM_ASSERT(edge[0] != 255);
              VTKM_ASSERT(edge[1] != 255);
              EdgeInterpolation ei;
              ei.Vertex1 = points[edge[0]];
              ei.Vertex2 = points[edge[1]];
              // For consistency purposes keep the points ordered.
              if (ei.Vertex1 > ei.Vertex2)
              {
                this->swap(ei.Vertex1, ei.Vertex2);
                this->swap(edge[0], edge[1]);
              }
              ei.Weight = (static_cast<vtkm::Float64>(scalars[edge[0]]) - this->Value) /
                static_cast<vtkm::Float64>(scalars[edge[1]] - scalars[edge[0]]);
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
    VTKM_EXEC void swap(T& v1, T& v2) const
    {
      T temp = v1;
      v1 = v2;
      v2 = temp;
    }

  private:
    vtkm::Float64 Value;
  };

  class ScatterEdgeConnectivity : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    ScatterEdgeConnectivity(vtkm::Id edgePointOffset)
      : EdgePointOffset(edgePointOffset)
    {
    }

    using ControlSignature = void(FieldIn sourceValue,
                                  FieldIn destinationIndices,
                                  WholeArrayOut destinationData);

    using ExecutionSignature = void(_1, _2, _3);

    using InputDomain = _1;

    template <typename ConnectivityDataType>
    VTKM_EXEC void operator()(vtkm::Id sourceValue,
                              vtkm::Id destinationIndex,
                              ConnectivityDataType& destinationData) const
    {
      destinationData.Set(destinationIndex, (sourceValue + EdgePointOffset));
    }

  private:
    vtkm::Id EdgePointOffset;
  };

  class ScatterInCellConnectivity : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    ScatterInCellConnectivity(vtkm::Id inCellPointOffset)
      : InCellPointOffset(inCellPointOffset)
    {
    }

    using ControlSignature = void(FieldIn destinationIndices, WholeArrayOut destinationData);

    using ExecutionSignature = void(_1, _2);

    using InputDomain = _1;

    template <typename ConnectivityDataType>
    VTKM_EXEC void operator()(vtkm::Id destinationIndex,
                              ConnectivityDataType& destinationData) const
    {
      auto sourceValue = destinationData.Get(destinationIndex);
      destinationData.Set(destinationIndex, (sourceValue + InCellPointOffset));
    }

  private:
    vtkm::Id InCellPointOffset;
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
  vtkm::cont::CellSetExplicit<> Run(const CellSetType& cellSet,
                                    const ScalarsArrayHandle& scalars,
                                    vtkm::Float64 value,
                                    bool invert)
  {
    vtkm::cont::Invoker invoke;

    // Create the required output fields.
    vtkm::cont::ArrayHandle<ClipStats> clipStats;
    vtkm::cont::ArrayHandle<vtkm::Id> clipTableIndices;

    //Send this CellSet to process
    ComputeStats statsWorklet(value, invert);
    invoke(statsWorklet, cellSet, scalars, this->ClipTablesInstance, clipStats, clipTableIndices);

    ClipStats zero;
    vtkm::cont::ArrayHandle<ClipStats> cellSetStats;
    ClipStats total =
      vtkm::cont::Algorithm::ScanExclusive(clipStats, cellSetStats, ClipStats::SumOp(), zero);
    clipStats.ReleaseResources();

    vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numberOfIndices;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    vtkm::cont::ArrayHandle<vtkm::Id> offsets;
    internal::ConnectivityExplicit connectivityObject(
      shapes, numberOfIndices, connectivity, offsets, total);

    //Begin Process of Constructing the new CellSet.
    vtkm::cont::ArrayHandle<vtkm::Id> pointsOnlyConnectivityIndices;
    pointsOnlyConnectivityIndices.Allocate(total.NumberOfIndices - total.NumberOfEdgeIndices -
                                           total.NumberOfInCellIndices);

    vtkm::cont::ArrayHandle<vtkm::Id> edgePointReverseConnectivity;
    edgePointReverseConnectivity.Allocate(total.NumberOfEdgeIndices);
    vtkm::cont::ArrayHandle<EdgeInterpolation> edgeInterpolation;
    edgeInterpolation.Allocate(total.NumberOfEdgeIndices);

    vtkm::cont::ArrayHandle<vtkm::Id> cellPointReverseConnectivity;
    cellPointReverseConnectivity.Allocate(total.NumberOfInCellIndices);
    vtkm::cont::ArrayHandle<vtkm::Id> cellPointEdgeReverseConnectivity;
    cellPointEdgeReverseConnectivity.Allocate(total.NumberOfInCellEdgeIndices);
    vtkm::cont::ArrayHandle<EdgeInterpolation> cellPointEdgeInterpolation;
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
      vtkm::cont::ArrayHandle<vtkm::IdComponent> pointMask;
      pointMask.AllocateAndFill(scalars.GetNumberOfValues(), 0);

      auto pointsOnlyConnectivity =
        vtkm::cont::make_ArrayHandlePermutation(pointsOnlyConnectivityIndices, connectivity);

      invoke(
        vtkm::worklet::RemoveUnusedPoints::GeneratePointMask{}, pointsOnlyConnectivity, pointMask);

      vtkm::worklet::ScatterCounting scatter(pointMask, true);
      auto pointMapInputToOutput = scatter.GetInputToOutputMap();
      this->PointMapOutputToInput = scatter.GetOutputToInputMap();
      pointMask.ReleaseResources();

      invoke(vtkm::worklet::RemoveUnusedPoints::TransformPointIndices{},
             pointsOnlyConnectivity,
             pointMapInputToOutput,
             pointsOnlyConnectivity);

      pointsOnlyConnectivityIndices.ReleaseResources();

      // We want to find the entries in `InCellInterpolationInfo` that point to exisiting points.
      // `cellPointEdgeReverseConnectivity` map to entries that point to edges.
      vtkm::cont::ArrayHandle<vtkm::UInt8> stencil;
      stencil.AllocateAndFill(this->InCellInterpolationInfo.GetNumberOfValues(), 1);
      auto edgeOnlyStencilEntries =
        vtkm::cont::make_ArrayHandlePermutation(cellPointEdgeReverseConnectivity, stencil);
      vtkm::cont::Algorithm::Fill(edgeOnlyStencilEntries, vtkm::UInt8{});
      vtkm::cont::ArrayHandle<vtkm::Id> idxsToPoints;
      vtkm::cont::Algorithm::CopyIf(
        vtkm::cont::ArrayHandleIndex(this->InCellInterpolationInfo.GetNumberOfValues()),
        stencil,
        idxsToPoints);
      stencil.ReleaseResources();

      // Remap the point indices in `InCellInterpolationInfo`, to the used-only point indices
      // computed above.
      // This only works if the points needed for interpolating centroids are included in the
      // `connectivity` array. This has been verified to be true for all cases in the clip tables.
      auto inCellInterpolationInfoPointsOnly =
        vtkm::cont::make_ArrayHandlePermutation(idxsToPoints, this->InCellInterpolationInfo);
      invoke(vtkm::worklet::RemoveUnusedPoints::TransformPointIndices{},
             inCellInterpolationInfoPointsOnly,
             pointMapInputToOutput,
             inCellInterpolationInfoPointsOnly);
    }

    // Get unique EdgeInterpolation : unique edge points.
    // LowerBound for edgeInterpolation : get index into new edge points array.
    // LowerBound for cellPointEdgeInterpolation : get index into new edge points array.
    vtkm::cont::Algorithm::SortByKey(
      edgeInterpolation, edgePointReverseConnectivity, EdgeInterpolation::LessThanOp());
    vtkm::cont::Algorithm::Copy(edgeInterpolation, this->EdgePointsInterpolation);
    vtkm::cont::Algorithm::Unique(this->EdgePointsInterpolation, EdgeInterpolation::EqualToOp());

    vtkm::cont::ArrayHandle<vtkm::Id> edgeInterpolationIndexToUnique;
    vtkm::cont::Algorithm::LowerBounds(this->EdgePointsInterpolation,
                                       edgeInterpolation,
                                       edgeInterpolationIndexToUnique,
                                       EdgeInterpolation::LessThanOp());
    edgeInterpolation.ReleaseResources();

    // This only works if the edges in `cellPointEdgeInterpolation` also exist in
    // `EdgePointsInterpolation`. This has been verified to be true for all cases in the clip
    // tables.
    vtkm::cont::ArrayHandle<vtkm::Id> cellInterpolationIndexToUnique;
    vtkm::cont::Algorithm::LowerBounds(this->EdgePointsInterpolation,
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

    vtkm::cont::CellSetExplicit<> output;
    vtkm::Id numberOfPoints = this->PointMapOutputToInput.GetNumberOfValues() +
      this->EdgePointsInterpolation.GetNumberOfValues() + total.NumberOfInCellPoints;

    vtkm::cont::ConvertNumComponentsToOffsets(numberOfIndices, offsets);

    output.Fill(numberOfPoints, shapes, connectivity, offsets);
    return output;
  }

  template <typename CellSetType, typename ImplicitFunction>
  class ClipWithImplicitFunction
  {
  public:
    VTKM_CONT
    ClipWithImplicitFunction(Clip* clipper,
                             const CellSetType& cellSet,
                             const ImplicitFunction& function,
                             vtkm::Float64 offset,
                             bool invert,
                             vtkm::cont::CellSetExplicit<>* result)
      : Clipper(clipper)
      , CellSet(&cellSet)
      , Function(function)
      , Offset(offset)
      , Invert(invert)
      , Result(result)
    {
    }

    template <typename ArrayHandleType>
    VTKM_CONT void operator()(const ArrayHandleType& handle) const
    {
      // Evaluate the implicit function on the input coordinates using
      // ArrayHandleTransform
      vtkm::cont::ArrayHandleTransform<ArrayHandleType,
                                       vtkm::ImplicitFunctionValueFunctor<ImplicitFunction>>
        clipScalars(handle, this->Function);

      // Clip at locations where the implicit function evaluates to `Offset`
      *this->Result = this->Clipper->Run(*this->CellSet, clipScalars, this->Offset, this->Invert);
    }

  private:
    Clip* Clipper;
    const CellSetType* CellSet;
    ImplicitFunction Function;
    vtkm::Float64 Offset;
    bool Invert;
    vtkm::cont::CellSetExplicit<>* Result;
  };

  template <typename CellSetType, typename ImplicitFunction>
  vtkm::cont::CellSetExplicit<> Run(const CellSetType& cellSet,
                                    const ImplicitFunction& clipFunction,
                                    vtkm::Float64 offset,
                                    const vtkm::cont::CoordinateSystem& coords,
                                    bool invert)
  {
    vtkm::cont::CellSetExplicit<> output;

    ClipWithImplicitFunction<CellSetType, ImplicitFunction> clip(
      this, cellSet, clipFunction, offset, invert, &output);

    CastAndCall(coords, clip);
    return output;
  }

  template <typename CellSetType, typename ImplicitFunction>
  vtkm::cont::CellSetExplicit<> Run(const CellSetType& cellSet,
                                    const ImplicitFunction& clipFunction,
                                    const vtkm::cont::CoordinateSystem& coords,
                                    bool invert)
  {
    return this->Run(cellSet, clipFunction, 0.0, coords, invert);
  }

  struct PerformEdgeInterpolations : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn edgeInterpolations,
                                  WholeArrayIn originalField,
                                  FieldOut outputField);
    using ExecutionSignature = void(_1, _2, _3);

    template <typename FieldPortal, typename T>
    VTKM_EXEC void operator()(const EdgeInterpolation& edgeInterp,
                              const FieldPortal& originalField,
                              T& output) const
    {
      T v1 = originalField.Get(edgeInterp.Vertex1);
      T v2 = originalField.Get(edgeInterp.Vertex2);

      // Interpolate per-vertex because some vec-like objects do not allow intermediate variables
      using VTraits = vtkm::VecTraits<T>;
      using CType = typename VTraits::ComponentType;
      VTKM_ASSERT(VTraits::GetNumberOfComponents(v1) == VTraits::GetNumberOfComponents(output));
      VTKM_ASSERT(VTraits::GetNumberOfComponents(v2) == VTraits::GetNumberOfComponents(output));
      for (vtkm::IdComponent component = 0; component < VTraits::GetNumberOfComponents(output);
           ++component)
      {
        CType c1 = VTraits::GetComponent(v1, component);
        CType c2 = VTraits::GetComponent(v2, component);
        CType o = static_cast<CType>(((c1 - c2) * edgeInterp.Weight) + c1);
        VTraits::SetComponent(output, component, o);
      }
    }
  };

  struct PerformInCellInterpolations : public vtkm::worklet::WorkletReduceByKey
  {
    using ControlSignature = void(KeysIn keys, ValuesIn toReduce, ReducedValuesOut centroids);
    using ExecutionSignature = void(_2, _3);

    template <typename MappedValueVecType, typename MappedValueType>
    VTKM_EXEC void operator()(const MappedValueVecType& toReduce, MappedValueType& centroid) const
    {
      const vtkm::IdComponent numValues = toReduce.GetNumberOfComponents();

      // Interpolate per-vertex because some vec-like objects do not allow intermediate variables
      using VTraits = vtkm::VecTraits<MappedValueType>;
      using CType = typename VTraits::ComponentType;
      for (vtkm::IdComponent component = 0; component < VTraits::GetNumberOfComponents(centroid);
           ++component)
      {
        CType sum = VTraits::GetComponent(toReduce[0], component);
        for (vtkm::IdComponent reduceI = 1; reduceI < numValues; ++reduceI)
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

    vtkm::Id numberOfVertexPoints = this->PointMapOutputToInput.GetNumberOfValues();
    vtkm::Id numberOfEdgePoints = this->EdgePointsInterpolation.GetNumberOfValues();
    vtkm::Id numberOfInCellPoints = this->InterpolationKeys.GetUniqueKeys().GetNumberOfValues();

    output.Allocate(numberOfVertexPoints + numberOfEdgePoints + numberOfInCellPoints);

    // Copy over the original values that are still part of the output.
    vtkm::cont::Algorithm::CopySubRange(
      vtkm::cont::make_ArrayHandlePermutation(this->PointMapOutputToInput, input),
      0,
      numberOfVertexPoints,
      output);

    // Interpolate all new points that lie on edges of the input mesh.
    vtkm::cont::Invoker invoke;
    invoke(PerformEdgeInterpolations{},
           this->EdgePointsInterpolation,
           input,
           vtkm::cont::make_ArrayHandleView(output, numberOfVertexPoints, numberOfEdgePoints));

    // Perform a gather on the output to get all the required values for calculation of centroids
    // using the interpolation info array.
    auto toReduceValues =
      vtkm::cont::make_ArrayHandlePermutation(this->InCellInterpolationInfo, output);
    invoke(PerformInCellInterpolations{},
           this->InterpolationKeys,
           toReduceValues,
           vtkm::cont::make_ArrayHandleView(
             output, numberOfVertexPoints + numberOfEdgePoints, numberOfInCellPoints));
  }

  vtkm::cont::ArrayHandle<vtkm::Id> GetCellMapOutputToInput() const
  {
    return this->CellMapOutputToInput;
  }

private:
  internal::ClipTables ClipTablesInstance;
  vtkm::cont::ArrayHandle<EdgeInterpolation> EdgePointsInterpolation;
  vtkm::cont::ArrayHandle<vtkm::Id> InCellInterpolationKeys;
  vtkm::cont::ArrayHandle<vtkm::Id> InCellInterpolationInfo;
  vtkm::cont::ArrayHandle<vtkm::Id> CellMapOutputToInput;
  vtkm::cont::ArrayHandle<vtkm::Id> PointMapOutputToInput;
  vtkm::Id EdgePointsOffset;
  vtkm::Id InCellPointsOffset;
  vtkm::worklet::Keys<vtkm::Id> InterpolationKeys;
  bool InterpolationKeysBuilt = false;
};
}
} // namespace vtkm::worklet

#if defined(THRUST_SCAN_WORKAROUND)
namespace thrust
{
namespace detail
{

// causes a different code path which does not have the bug
template <>
struct is_integral<vtkm::worklet::ClipStats> : public true_type
{
};
}
} // namespace thrust::detail
#endif

#endif // vtkm_m_worklet_Clip_h
