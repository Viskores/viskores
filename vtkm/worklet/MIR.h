//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef vtk_m_worklet_MIR_h
#define vtk_m_worklet_MIR_h

#include <vtkm/CellShape.h>
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/DispatcherReduceByKey.h>
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

#include <vtkm/filter/contour/worklet/clip/ClipTables.h>
#include <vtkm/worklet/mir/MIRTables.h>

namespace vtkm
{
namespace worklet
{

struct MIRStats
{
  vtkm::Id NumberOfCells = 0;
  vtkm::Id NumberOfIndices = 0;
  vtkm::Id NumberOfEdgeIndices = 0;

  //VTKM New point stats
  vtkm::Id NumberOfInCellPoints = 0;
  vtkm::Id NumberOfInCellIndices = 0;
  vtkm::Id NumberOfInCellInterpPoints = 0;
  vtkm::Id NumberOfInCellEdgeIndices = 0;

  struct SumOp
  {
    VTKM_EXEC_CONT
    MIRStats operator()(const MIRStats& stat1, const MIRStats& stat2) const
    {
      MIRStats sum = stat1;
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
namespace MIRinternal
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
}

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
                                MIRStats stats,
                                vtkm::cont::DeviceAdapterId device,
                                vtkm::cont::Token& token)
    : Shapes(shapes.PrepareForOutput(stats.NumberOfCells, device, token))
    , NumberOfIndices(numberOfIndices.PrepareForOutput(stats.NumberOfCells, device, token))
    , Connectivity(connectivity.PrepareForOutput(stats.NumberOfIndices, device, token))
    , Offsets(offsets.PrepareForOutput(stats.NumberOfCells, device, token))
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
                       const MIRStats& stats)
    : Shapes(shapes)
    , NumberOfIndices(numberOfIndices)
    , Connectivity(connectivity)
    , Offsets(offsets)
    , Stats(stats)
  {
  }

  VTKM_CONT ExecutionConnectivityExplicit PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                              vtkm::cont::Token& token) const
  {
    ExecutionConnectivityExplicit execConnectivity(this->Shapes,
                                                   this->NumberOfIndices,
                                                   this->Connectivity,
                                                   this->Offsets,
                                                   this->Stats,
                                                   device,
                                                   token);
    return execConnectivity;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::UInt8> Shapes;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumberOfIndices;
  vtkm::cont::ArrayHandle<vtkm::Id> Connectivity;
  vtkm::cont::ArrayHandle<vtkm::Id> Offsets;
  vtkm::worklet::MIRStats Stats;
};

class ComputeStats : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  vtkm::Id targ;
  VTKM_CONT ComputeStats(vtkm::Id target)
    : targ(target)
  {
  }
  VTKM_CONT ComputeStats() = default;
  using ControlSignature = void(CellSetIn,
                                WholeArrayIn curVals,
                                WholeArrayIn prevVals,
                                FieldInCell offsets,
                                ExecObject mirTables,
                                FieldInCell parentObj,
                                FieldInCell prevCol,
                                FieldOutCell stats,
                                FieldOutCell caseID);
  using ExecutionSignature = void(CellShape, PointCount, _3, _2, _4, _5, _6, _7, _8, _9);
  using InputDomain = _1;

  template <typename CellShapeTag,
            typename ScalarFieldVec,
            typename ScalarFieldVec1,
            typename DeviceAdapter,
            typename ScalarPos,
            typename ParentObj,
            typename PreCol>
  VTKM_EXEC void operator()(
    const CellShapeTag shape,
    const vtkm::IdComponent pointCount,
    const ScalarFieldVec& prevVals,
    const ScalarFieldVec1& newVals,
    const ScalarPos& valPositionStart,
    const vtkm::worklet::MIRCases::MIRTables::MIRDevicePortal<DeviceAdapter>& MIRData,
    const ParentObj&,
    const PreCol& prevCol,
    MIRStats& MIRStat,
    vtkm::Id& MIRDataIndex) const
  {
    (void)shape;
    vtkm::Id caseId = 0;
    if (prevCol == vtkm::Id(-1))
    {
      // In case of this being the first material for the cell, automatically set it to the furthest case (that is, same shape, color 1)
      for (vtkm::IdComponent iter = pointCount - 1; iter >= 0; iter--)
      {
        caseId++;
        if (iter > 0)
        {
          caseId *= 2;
        }
      }
    }
    else
    {
      for (vtkm::IdComponent iter = pointCount - 1; iter >= 0; iter--)
      {
        if (static_cast<vtkm::Float64>(prevVals[valPositionStart + iter]) <=
            static_cast<vtkm::Float64>(newVals[valPositionStart + iter]))
        {
          caseId++;
        }
        if (iter > 0)
        {
          caseId *= 2;
        }
      }
    }
    // Reinitialize all struct values to 0, experienced weird memory bug otherwise, might be an issue with development environment
    MIRStat.NumberOfCells = 0;
    MIRStat.NumberOfEdgeIndices = 0;
    MIRStat.NumberOfInCellEdgeIndices = 0;
    MIRStat.NumberOfInCellIndices = 0;
    MIRStat.NumberOfInCellInterpPoints = 0;
    MIRStat.NumberOfInCellPoints = 0;
    MIRStat.NumberOfIndices = 0;
    vtkm::Id index = MIRData.GetCaseIndex(shape.Id, caseId, pointCount);
    MIRDataIndex = vtkm::Id(caseId);
    vtkm::Id numberOfCells = MIRData.GetNumberOfShapes(shape.Id, caseId, pointCount);
    if (numberOfCells == -1)
    {
      this->RaiseError("Getting a size index of a polygon with more points than 8 or less points "
                       "than 3. Bad case.");
      return;
    }
    MIRStat.NumberOfCells = numberOfCells;

    for (vtkm::IdComponent shapes = 0; shapes < numberOfCells; shapes++)
    {
      vtkm::UInt8 cellType = MIRData.ValueAt(index++);
      // SH_PNT is a specification that a center point is to be used
      // Note: It is only possible to support 1 midpoint with the current code format
      if (cellType == MIRCases::SH_PNT)
      {
        MIRStat.NumberOfCells = numberOfCells - 1;
        vtkm::UInt8 numberOfIndices = MIRData.ValueAt(index + 2);
        index += 3;
        MIRStat.NumberOfInCellPoints = 1;
        MIRStat.NumberOfInCellInterpPoints = numberOfIndices;
        for (vtkm::IdComponent points = 0; points < numberOfIndices; points++)
        {
          vtkm::Id elem = MIRData.ValueAt(index);
          // If the midpoint needs to reference an edge point, record it.
          MIRStat.NumberOfInCellEdgeIndices += (elem >= MIRCases::EA) ? 1 : 0;
          index++;
        }
      }
      else
      {
        vtkm::Id numberOfIndices = MIRData.GetNumberOfIndices(cellType);
        index++;
        MIRStat.NumberOfIndices += numberOfIndices;
        for (vtkm::IdComponent points = 0; points < numberOfIndices; points++, index++)
        {
          vtkm::IdComponent element = MIRData.ValueAt(index);
          if (element >= MIRCases::EA && element <= MIRCases::EL)
          {
            MIRStat.NumberOfEdgeIndices++;
          }
          else if (element == MIRCases::N0)
          {
            // N0 stands for the midpoint. Technically it could be N0->N3, but with the current
            // setup, only N0 is supported/present in the MIRCases tables.
            MIRStat.NumberOfInCellIndices++;
          }
        }
      }
    }
  }
};
class MIRParentObject : public vtkm::cont::ExecutionAndControlObjectBase
{
public:
  VTKM_CONT MIRParentObject() = default;
  VTKM_CONT MIRParentObject(vtkm::Id numCells,
                            vtkm::cont::ArrayHandle<vtkm::Id> celllook,
                            vtkm::cont::ArrayHandle<vtkm::Id> cellCol,
                            vtkm::cont::ArrayHandle<vtkm::Id> newCellCol,
                            vtkm::cont::ArrayHandle<vtkm::Id> newcellLook)
    : newCellColors(newCellCol)
    , newCellLookback(newcellLook)
    , numberOfInd(numCells)
    , cellLookback(celllook)
    , cellColors(cellCol){};

  class MIRParentPortal
  {
  public:
    VTKM_EXEC void SetNewCellLookback(vtkm::Id index, vtkm::Id originalIndex)
    {
      this->NewCellLookback.Set(index, originalIndex);
    }
    VTKM_EXEC void SetNewCellColor(vtkm::Id index, vtkm::Id col)
    {
      this->NewCellColors.Set(index, col);
    }
    VTKM_EXEC vtkm::Id GetParentCellIndex(vtkm::Id index) { return this->CellLookback.Get(index); }
    VTKM_EXEC vtkm::Id GetParentCellColor(vtkm::Id index) { return this->CellColors.Get(index); }

  private:
    typename vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType CellLookback;
    typename vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType CellColors;
    typename vtkm::cont::ArrayHandle<vtkm::Id>::WritePortalType NewCellColors;
    typename vtkm::cont::ArrayHandle<vtkm::Id>::WritePortalType NewCellLookback;
    friend class MIRParentObject;
  };

  VTKM_CONT MIRParentPortal PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                vtkm::cont::Token& token)
  {
    MIRParentPortal dev;
    dev.CellLookback = this->cellLookback.PrepareForInput(device, token);
    dev.CellColors = this->cellColors.PrepareForInput(device, token);
    dev.NewCellColors = this->newCellColors.PrepareForOutput(this->numberOfInd, device, token);
    dev.NewCellLookback = this->newCellLookback.PrepareForOutput(this->numberOfInd, device, token);
    return dev;
  }
  vtkm::cont::ArrayHandle<vtkm::Id> newCellColors;
  vtkm::cont::ArrayHandle<vtkm::Id> newCellLookback;

private:
  vtkm::Id numberOfInd;
  vtkm::cont::ArrayHandle<vtkm::Id> cellLookback;
  vtkm::cont::ArrayHandle<vtkm::Id> cellColors;
};
class GenerateCellSet : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  VTKM_EXEC_CONT
  GenerateCellSet(vtkm::Id tar)
    : target(tar)
  {
  }

  using ControlSignature = void(CellSetIn,
                                WholeArrayIn prevVals,
                                WholeArrayIn newVals,
                                FieldInCell vf_pos,
                                FieldInCell mirTableIndices,
                                FieldInCell mirStats,
                                ExecObject mirTables,
                                ExecObject connectivityObject,
                                WholeArrayOut edgePointReverseConnectivity,
                                WholeArrayOut edgePointInterpolation,
                                WholeArrayOut inCellReverseConnectivity,
                                WholeArrayOut inCellEdgeReverseConnectivity,
                                WholeArrayOut inCellEdgeInterpolation,
                                WholeArrayOut inCellInterpolationKeys,
                                WholeArrayOut inCellInterpolationInfo,
                                ExecObject cellLookbackObj,
                                WholeArrayOut simpleLookback);

  using ExecutionSignature = void(CellShape,
                                  InputIndex,
                                  PointCount,
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
                                  _15,
                                  _16,
                                  _17); // 20! NO MORE ROOM!

  template <typename CellShapeTag,
            typename PointVecType,
            typename ScalarVecType1,
            typename ScalarVecType2,
            typename ConnectivityObject,
            typename IdArrayType,
            typename EdgeInterpolationPortalType,
            typename DeviceAdapter,
            typename ScalarPos,
            typename CellLookbackArr>
  VTKM_EXEC void operator()(
    const CellShapeTag shape,
    const vtkm::Id workIndex,
    const vtkm::IdComponent pointcount,
    const PointVecType points,
    const ScalarVecType1& curScalars,  // Previous VF
    const ScalarVecType2& newScalars,  // New VF
    const ScalarPos& valPositionStart, // Offsets into the ^ arrays for indexing
    const vtkm::Id& clipDataIndex,
    const MIRStats mirStats,
    const worklet::MIRCases::MIRTables::MIRDevicePortal<DeviceAdapter>& MIRData,
    ConnectivityObject& connectivityObject,
    IdArrayType& edgePointReverseConnectivity,
    EdgeInterpolationPortalType& edgePointInterpolation,
    IdArrayType& inCellReverseConnectivity,
    IdArrayType& inCellEdgeReverseConnectivity,
    EdgeInterpolationPortalType& inCellEdgeInterpolation,
    IdArrayType& inCellInterpolationKeys,
    IdArrayType& inCellInterpolationInfo,
    worklet::MIRParentObject::MIRParentPortal& parentObj,
    CellLookbackArr& cellLookbackArray) const
  {

    (void)shape;
    vtkm::Id clipIndex = MIRData.GetCaseIndex(shape.Id, clipDataIndex, pointcount);

    // Start index for the cells of this case.
    vtkm::Id cellIndex = mirStats.NumberOfCells;
    // Start index to store connevtivity of this case.
    vtkm::Id connectivityIndex = mirStats.NumberOfIndices;
    // Start indices for reverse mapping into connectivity for this case.
    vtkm::Id edgeIndex = mirStats.NumberOfEdgeIndices;
    vtkm::Id inCellIndex = mirStats.NumberOfInCellIndices;
    vtkm::Id inCellPoints = mirStats.NumberOfInCellPoints;
    // Start Indices to keep track of interpolation points for new cell.
    vtkm::Id inCellInterpPointIndex = mirStats.NumberOfInCellInterpPoints;
    vtkm::Id inCellEdgeInterpIndex = mirStats.NumberOfInCellEdgeIndices;

    // Iterate over the shapes for the current cell and begin to fill connectivity.
    vtkm::Id numberOfCells = MIRData.GetNumberOfShapes(shape.Id, clipDataIndex, pointcount);

    for (vtkm::Id cell = 0; cell < numberOfCells; ++cell)
    {
      vtkm::UInt8 cellShape = MIRData.ValueAt(clipIndex++);
      if (cellShape == MIRCases::SH_PNT)
      {
        clipIndex += 2;
        vtkm::IdComponent numberOfPoints = MIRData.ValueAt(clipIndex);
        clipIndex++;
        // Case for a new cell point

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
          vtkm::IdComponent entry = static_cast<vtkm::IdComponent>(MIRData.ValueAt(clipIndex));
          inCellInterpolationKeys.Set(inCellInterpPointIndex, workIndex);
          if (entry <= MIRCases::P7)
          {
            inCellInterpolationInfo.Set(inCellInterpPointIndex, points[entry]);
          }
          else
          {
            internal::ClipTables::EdgeVec edge =
              MIRData.GetEdge(shape.Id, entry - MIRCases::EA, pointcount);
            if (edge[0] == 255 || edge[1] == 255)
            {
              this->RaiseError("Edge vertices are assigned incorrect values.");
              return;
            }

            EdgeInterpolation ei;
            ei.Vertex1 = points[edge[0]];
            ei.Vertex2 = points[edge[1]];
            // For consistency purposes keep the points ordered.
            if (ei.Vertex1 > ei.Vertex2)
            {
              this->swap(ei.Vertex1, ei.Vertex2);
              this->swap(edge[0], edge[1]);
            }
            // need to swap the weight of the point to be A-C / ((D-C) - (B-A)),
            // where A and C are edge0 mats 1 and 2, and B and D are edge1 mats 1 and 2.
            ei.Weight = vtkm::Float64(1) +
              ((static_cast<vtkm::Float64>(curScalars[valPositionStart + edge[0]] -
                                           newScalars[valPositionStart + edge[0]])) /
               static_cast<vtkm::Float64>(
                 curScalars[valPositionStart + edge[1]] - curScalars[valPositionStart + edge[0]] +
                 newScalars[valPositionStart + edge[0]] - newScalars[valPositionStart + edge[1]]));

            inCellEdgeReverseConnectivity.Set(inCellEdgeInterpIndex, inCellInterpPointIndex);
            inCellEdgeInterpolation.Set(inCellEdgeInterpIndex, ei);
            inCellEdgeInterpIndex++;
          }
        }
      }
      else
      {
        vtkm::IdComponent numberOfPoints =
          static_cast<vtkm::IdComponent>(MIRData.GetNumberOfIndices(cellShape));
        vtkm::IdComponent colorQ = static_cast<vtkm::IdComponent>(MIRData.ValueAt(clipIndex++));
        vtkm::Id color = colorQ == vtkm::IdComponent(MIRCases::COLOR0)
          ? parentObj.GetParentCellColor(workIndex)
          : target;
        parentObj.SetNewCellColor(cellIndex, color);
        parentObj.SetNewCellLookback(cellIndex, parentObj.GetParentCellIndex(workIndex));
        connectivityObject.SetCellShape(cellIndex, cellShape);
        connectivityObject.SetNumberOfIndices(cellIndex, numberOfPoints);
        connectivityObject.SetIndexOffset(cellIndex, connectivityIndex);

        for (vtkm::IdComponent point = 0; point < numberOfPoints; point++, clipIndex++)
        {
          vtkm::IdComponent entry = static_cast<vtkm::IdComponent>(MIRData.ValueAt(clipIndex));
          if (entry == MIRCases::N0) // case of cell point interpolation
          {
            // Add index of the corresponding cell point.
            inCellReverseConnectivity.Set(inCellIndex++, connectivityIndex);
            connectivityObject.SetConnectivity(connectivityIndex, inCellPoints);
            connectivityIndex++;
          }
          else if (entry <= MIRCases::P7) // existing vertex
          {
            connectivityObject.SetConnectivity(connectivityIndex, points[entry]);
            connectivityIndex++;
          }
          else // case of a new edge point
          {
            internal::ClipTables::EdgeVec edge =
              MIRData.GetEdge(shape.Id, entry - MIRCases::EA, pointcount);
            if (edge[0] == 255 || edge[1] == 255)
            {
              this->RaiseError("Edge vertices are assigned incorrect values.");
              return;
            }
            EdgeInterpolation ei;
            ei.Vertex1 = points[edge[0]];
            ei.Vertex2 = points[edge[1]];
            // For consistency purposes keep the points ordered.
            if (ei.Vertex1 > ei.Vertex2)
            {
              this->swap(ei.Vertex1, ei.Vertex2);
              this->swap(edge[0], edge[1]);
            }

            ei.Weight = vtkm::Float64(1) +
              ((static_cast<vtkm::Float64>(curScalars[valPositionStart + edge[0]] -
                                           newScalars[valPositionStart + edge[0]])) /
               static_cast<vtkm::Float64>(
                 curScalars[valPositionStart + edge[1]] - curScalars[valPositionStart + edge[0]] +
                 newScalars[valPositionStart + edge[0]] - newScalars[valPositionStart + edge[1]]));
            //Add to set of new edge points
            //Add reverse connectivity;
            edgePointReverseConnectivity.Set(edgeIndex, connectivityIndex++);
            edgePointInterpolation.Set(edgeIndex, ei);
            edgeIndex++;
          }
        }
        // Set cell matID...
        cellLookbackArray.Set(cellIndex, workIndex);
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
  vtkm::Id target;
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
  VTKM_EXEC void operator()(const vtkm::Id sourceValue,
                            const vtkm::Id destinationIndex,
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
  VTKM_EXEC void operator()(const vtkm::Id destinationIndex,
                            ConnectivityDataType& destinationData) const
  {
    auto sourceValue = destinationData.Get(destinationIndex);
    destinationData.Set(destinationIndex, (sourceValue + InCellPointOffset));
  }

private:
  vtkm::Id InCellPointOffset;
};
class MIR
{
public:
  MIR()
    : MIRTablesInstance()
    , EdgePointsInterpolation()
    , InCellInterpolationKeys()
    , InCellInterpolationInfo()
    , CellMapOutputToInput()
    , EdgePointsOffset()
    , InCellPointsOffset()
  {
  }
  template <typename VFList1, typename VFList2, typename CellSet, typename VFLocs, typename IDList>
  vtkm::cont::CellSetExplicit<> Run(const CellSet& cellSet,
                                    const VFList1& prevValues,
                                    const VFList2& curValues,
                                    const VFLocs& offsets,
                                    const IDList& prevIDs,
                                    const vtkm::Id& newID,
                                    const IDList& prevLookback,
                                    IDList& newIDs,
                                    IDList& newLookback)
  {
    // First compute the stats for the MIR algorithm & build the offsets
    //{
    ComputeStats statWorklet(newID);
    vtkm::worklet::DispatcherMapTopology<ComputeStats> statsDispatch(statWorklet);

    // Output variables
    vtkm::cont::ArrayHandle<MIRStats> mirStats;
    vtkm::cont::ArrayHandle<vtkm::Id> mirInd;

    statsDispatch.Invoke(cellSet,
                         curValues,
                         prevValues,
                         offsets,
                         this->MIRTablesInstance,
                         prevLookback,
                         prevIDs,
                         mirStats,
                         mirInd);
    // Sum all stats to form an offset array (for indexing in the MIR algorithm)
    MIRStats zero;
    vtkm::cont::ArrayHandle<MIRStats> cellSetStats;
    MIRStats total =
      vtkm::cont::Algorithm::ScanExclusive(mirStats, cellSetStats, MIRStats::SumOp(), zero);
    mirStats.ReleaseResources();
    //}
    // Secondly, build the sets.
    //{
    // CellSetExplicit sets
    vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numberOfIndices;
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    vtkm::cont::ArrayHandle<vtkm::Id> offset;
    ConnectivityExplicit connectivityObject(shapes, numberOfIndices, connectivity, offset, total);
    // Connectivity related sets
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


    //}
    // Thirdly, call the MIR generator
    //{
    GenerateCellSet cellSetWorklet(newID);
    vtkm::worklet::DispatcherMapTopology<GenerateCellSet> cellSetDispatcher(cellSetWorklet);
    // Output arrays storing information about cell lookbacks and cell material IDs
    vtkm::cont::ArrayHandle<vtkm::Id> nextID, nextLookback;
    nextID.Allocate(total.NumberOfCells);
    nextLookback.Allocate(total.NumberOfCells);
    MIRParentObject po(total.NumberOfCells, prevLookback, prevIDs, nextID, nextLookback);


    // Perform the MIR step
    cellSetDispatcher.Invoke(cellSet,
                             prevValues,
                             curValues,
                             offsets,
                             mirInd,
                             cellSetStats,
                             this->MIRTablesInstance,
                             connectivityObject,
                             edgePointReverseConnectivity,
                             edgeInterpolation,
                             cellPointReverseConnectivity,
                             cellPointEdgeReverseConnectivity,
                             cellPointEdgeInterpolation,
                             this->InCellInterpolationKeys,
                             this->InCellInterpolationInfo,
                             po,
                             this->CellMapOutputToInput);

    //}
    // Forthly, create the output set and clean up connectivity objects.
    //{
    // Get unique keys for all shared edges
    vtkm::cont::Algorithm::SortByKey(
      edgeInterpolation, edgePointReverseConnectivity, EdgeInterpolation::LessThanOp());
    vtkm::cont::Algorithm::Copy(edgeInterpolation, this->EdgePointsInterpolation);
    vtkm::cont::Algorithm::Unique(this->EdgePointsInterpolation, EdgeInterpolation::EqualToOp());
    vtkm::cont::ArrayHandle<vtkm::Id> edgeInterpolationIndexToUnique;
    vtkm::cont::Algorithm::LowerBounds(this->EdgePointsInterpolation,
                                       edgeInterpolation,
                                       edgeInterpolationIndexToUnique,
                                       EdgeInterpolation::LessThanOp());

    vtkm::cont::ArrayHandle<vtkm::Id> cellInterpolationIndexToUnique;
    vtkm::cont::Algorithm::LowerBounds(this->EdgePointsInterpolation,
                                       cellPointEdgeInterpolation,
                                       cellInterpolationIndexToUnique,
                                       EdgeInterpolation::LessThanOp());
    this->EdgePointsOffset = cellSet.GetNumberOfPoints();
    this->InCellPointsOffset =
      this->EdgePointsOffset + this->EdgePointsInterpolation.GetNumberOfValues();

    ScatterEdgeConnectivity scatterEdgePointConnectivity(this->EdgePointsOffset);
    vtkm::worklet::DispatcherMapField<ScatterEdgeConnectivity> scatterEdgeDispatcher(
      scatterEdgePointConnectivity);
    scatterEdgeDispatcher.Invoke(
      edgeInterpolationIndexToUnique, edgePointReverseConnectivity, connectivity);
    scatterEdgeDispatcher.Invoke(cellInterpolationIndexToUnique,
                                 cellPointEdgeReverseConnectivity,
                                 this->InCellInterpolationInfo);
    // Add offset in connectivity of all new in-cell points.
    ScatterInCellConnectivity scatterInCellPointConnectivity(this->InCellPointsOffset);
    vtkm::worklet::DispatcherMapField<ScatterInCellConnectivity> scatterInCellDispatcher(
      scatterInCellPointConnectivity);
    scatterInCellDispatcher.Invoke(cellPointReverseConnectivity, connectivity);

    vtkm::cont::CellSetExplicit<> output;
    vtkm::Id numberOfPoints = cellSet.GetNumberOfPoints() +
      this->EdgePointsInterpolation.GetNumberOfValues() + total.NumberOfInCellPoints;

    vtkm::cont::ConvertNumComponentsToOffsets(numberOfIndices, offset);
    // Create explicit cell set output
    output.Fill(numberOfPoints, shapes, connectivity, offset);
    //}
    vtkm::cont::ArrayCopy(po.newCellColors, newIDs);
    vtkm::cont::ArrayCopy(po.newCellLookback, newLookback);

    return output;
  }

  template <typename ArrayHandleType>
  class InterpolateField
  {
  public:
    using ValueType = typename ArrayHandleType::ValueType;
    using TypeMappedValue = vtkm::List<ValueType>;

    InterpolateField(vtkm::cont::ArrayHandle<EdgeInterpolation> edgeInterpolationArray,
                     vtkm::cont::ArrayHandle<vtkm::Id> inCellInterpolationKeys,
                     vtkm::cont::ArrayHandle<vtkm::Id> inCellInterpolationInfo,
                     vtkm::Id edgePointsOffset,
                     vtkm::Id inCellPointsOffset,
                     ArrayHandleType* output)
      : EdgeInterpolationArray(edgeInterpolationArray)
      , InCellInterpolationKeys(inCellInterpolationKeys)
      , InCellInterpolationInfo(inCellInterpolationInfo)
      , EdgePointsOffset(edgePointsOffset)
      , InCellPointsOffset(inCellPointsOffset)
      , Output(output)
    {
    }

    class PerformEdgeInterpolations : public vtkm::worklet::WorkletMapField
    {
    public:
      PerformEdgeInterpolations(vtkm::Id edgePointsOffset)
        : EdgePointsOffset(edgePointsOffset)
      {
      }

      using ControlSignature = void(FieldIn edgeInterpolations, WholeArrayInOut outputField);

      using ExecutionSignature = void(_1, _2, WorkIndex);

      template <typename EdgeInterp, typename OutputFieldPortal>
      VTKM_EXEC void operator()(const EdgeInterp& ei,
                                OutputFieldPortal& field,
                                const vtkm::Id workIndex) const
      {
        using T = typename OutputFieldPortal::ValueType;
        T v1 = field.Get(ei.Vertex1);
        T v2 = field.Get(ei.Vertex2);
        field.Set(this->EdgePointsOffset + workIndex,
                  static_cast<T>(MIRinternal::Scale(T(v1 - v2), ei.Weight) + v2));
        if (ei.Weight > vtkm::Float64(1) || ei.Weight < vtkm::Float64(0))
        {
          this->RaiseError("Error in edge weight, assigned value not it interval [0,1].");
        }
      }

    private:
      vtkm::Id EdgePointsOffset;
    };

    class PerformInCellInterpolations : public vtkm::worklet::WorkletReduceByKey
    {
    public:
      using ControlSignature = void(KeysIn keys, ValuesIn toReduce, ReducedValuesOut centroid);

      using ExecutionSignature = void(_2, _3);

      template <typename MappedValueVecType, typename MappedValueType>
      VTKM_EXEC void operator()(const MappedValueVecType& toReduce, MappedValueType& centroid) const
      {
        vtkm::IdComponent numValues = toReduce.GetNumberOfComponents();
        MappedValueType sum = toReduce[0];
        for (vtkm::IdComponent i = 1; i < numValues; i++)
        {
          MappedValueType value = toReduce[i];
          // static_cast is for when MappedValueType is a small int that gets promoted to int32.
          sum = static_cast<MappedValueType>(sum + value);
        }
        centroid = MIRinternal::Scale(sum, 1. / static_cast<vtkm::Float64>(numValues));
      }
    };

    template <typename Storage>
    VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<ValueType, Storage>& field) const
    {
      vtkm::worklet::Keys<vtkm::Id> interpolationKeys(InCellInterpolationKeys);

      vtkm::Id numberOfOriginalValues = field.GetNumberOfValues();
      vtkm::Id numberOfEdgePoints = EdgeInterpolationArray.GetNumberOfValues();
      vtkm::Id numberOfInCellPoints = interpolationKeys.GetUniqueKeys().GetNumberOfValues();

      ArrayHandleType result;
      result.Allocate(numberOfOriginalValues + numberOfEdgePoints + numberOfInCellPoints);
      vtkm::cont::Algorithm::CopySubRange(field, 0, numberOfOriginalValues, result);

      PerformEdgeInterpolations edgeInterpWorklet(numberOfOriginalValues);
      vtkm::worklet::DispatcherMapField<PerformEdgeInterpolations> edgeInterpDispatcher(
        edgeInterpWorklet);
      edgeInterpDispatcher.Invoke(this->EdgeInterpolationArray, result);

      // Perform a gather on output to get all required values for calculation of
      // centroids using the interpolation info array.
      using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
      using ValueHandle = vtkm::cont::ArrayHandle<ValueType>;
      vtkm::cont::ArrayHandlePermutation<IdHandle, ValueHandle> toReduceValues(
        InCellInterpolationInfo, result);

      vtkm::cont::ArrayHandle<ValueType> reducedValues;
      vtkm::worklet::DispatcherReduceByKey<PerformInCellInterpolations>
        inCellInterpolationDispatcher;
      inCellInterpolationDispatcher.Invoke(interpolationKeys, toReduceValues, reducedValues);
      vtkm::Id inCellPointsOffset = numberOfOriginalValues + numberOfEdgePoints;
      vtkm::cont::Algorithm::CopySubRange(
        reducedValues, 0, reducedValues.GetNumberOfValues(), result, inCellPointsOffset);
      *(this->Output) = result;
    }

  private:
    vtkm::cont::ArrayHandle<EdgeInterpolation> EdgeInterpolationArray;
    vtkm::cont::ArrayHandle<vtkm::Id> InCellInterpolationKeys;
    vtkm::cont::ArrayHandle<vtkm::Id> InCellInterpolationInfo;
    vtkm::Id EdgePointsOffset;
    vtkm::Id InCellPointsOffset;
    ArrayHandleType* Output;
  };

  template <typename IDLen, typename IDPos, typename IDList, typename VFList>
  class InterpolateMIRFields
  {
  public:
    InterpolateMIRFields(vtkm::cont::ArrayHandle<EdgeInterpolation> edgeInterpolationArray,
                         vtkm::cont::ArrayHandle<vtkm::Id> inCellInterpolationKeys,
                         vtkm::cont::ArrayHandle<vtkm::Id> inCellInterpolationInfo,
                         vtkm::Id edgePointsOffset,
                         vtkm::Id inCellPointsOffset,
                         IDLen* output1,
                         IDPos* output2,
                         IDList* output3,
                         VFList* output4)
      : EdgeInterpolationArray(edgeInterpolationArray)
      , InCellInterpolationKeys(inCellInterpolationKeys)
      , InCellInterpolationInfo(inCellInterpolationInfo)
      , EdgePointsOffset(edgePointsOffset)
      , InCellPointsOffset(inCellPointsOffset)
      , LenOut(output1)
      , PosOut(output2)
      , IDOut(output3)
      , VFOut(output4)
    {
    }

    class PerformEdgeInterpolations : public vtkm::worklet::WorkletMapField
    {
    public:
      PerformEdgeInterpolations(vtkm::Id edgePointsOffset)
        : EdgePointsOffset(edgePointsOffset)
      {
      }

      using ControlSignature = void(FieldIn edgeInterpolations,
                                    WholeArrayIn lengths,
                                    WholeArrayIn positions,
                                    WholeArrayInOut ids,
                                    WholeArrayInOut vfs);

      using ExecutionSignature = void(_1, _2, _3, _4, _5, WorkIndex);

      template <typename EdgeInterp, typename IDL, typename IDO, typename IdsVec, typename VfsVec>
      VTKM_EXEC void operator()(const EdgeInterp& ei,
                                const IDL& lengths,
                                const IDO& positions,
                                IdsVec& ids,
                                VfsVec& vfs,
                                const vtkm::Id workIndex) const
      {
        vtkm::Vec<vtkm::Id, 2> idOff;
        vtkm::Vec<vtkm::Id, 2> idLen;
        vtkm::Vec<vtkm::Id, 2> idInd;
        vtkm::Vec<vtkm::Float64, 2> multiplier;
        multiplier[1] = vtkm::Float64(1.0) - ei.Weight;
        multiplier[0] = ei.Weight;
        vtkm::Id uniqueMats = vtkm::Id(0);

        idOff[0] = vtkm::Id(0);
        idOff[1] = idOff[0];
        idInd[0] = positions.Get(ei.Vertex1);
        idInd[1] = positions.Get(ei.Vertex2);
        idLen[0] = lengths.Get(ei.Vertex1);
        idLen[1] = lengths.Get(ei.Vertex2);
        vtkm::IdComponent numberOfPoints = 2;
        vtkm::UInt8 hasWork = vtkm::UInt8(1);
        while (hasWork != vtkm::UInt8(0))
        {
          hasWork = vtkm::UInt8(0);
          vtkm::Id lowest = vtkm::Id(-1);
          for (vtkm::IdComponent i = 0; i < numberOfPoints; i++)
          {
            if (idOff[i] < idLen[i])
            {
              vtkm::Id tmp = ids.Get(idInd[i] + idOff[i]);
              if (lowest == vtkm::Id(-1) || tmp < lowest)
              {
                lowest = tmp;
                hasWork = vtkm::UInt8(1);
              }
            }
          }
          if (hasWork != vtkm::UInt8(0))
          {
            vtkm::Float64 vfVal = vtkm::Float64(0);
            for (vtkm::IdComponent i = 0; i < numberOfPoints; i++)
            {
              if (idOff[i] < idLen[i])
              {
                vtkm::Id tmp = ids.Get(idInd[i] + idOff[i]);
                if (lowest == tmp)
                {
                  vfVal += multiplier[i] * vfs.Get(idInd[i] + idOff[i]);
                  idOff[i]++;
                }
              }
            }
            ids.Set(positions.Get(this->EdgePointsOffset + workIndex) + uniqueMats, lowest);
            vfs.Set(positions.Get(this->EdgePointsOffset + workIndex) + uniqueMats, vfVal);
            uniqueMats++;
          }
        }
      }

    private:
      vtkm::Id EdgePointsOffset;
    };
    class PerformEdgeInterpolations_C : public vtkm::worklet::WorkletMapField
    {
    private:
      vtkm::Id EdgePointsOffset;

    public:
      PerformEdgeInterpolations_C(vtkm::Id edgePointsOffset)
        : EdgePointsOffset(edgePointsOffset)
      {
      }
      using ControlSignature = void(FieldIn edgeInterpolations,
                                    WholeArrayInOut IDLengths,
                                    WholeArrayIn IDOffsets,
                                    WholeArrayIn IDs,
                                    FieldOut edgeLength);
      using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex, _5);
      template <typename EdgeInterp, typename IDL, typename IDO, typename IdsVec, typename ELL>
      VTKM_EXEC void operator()(const EdgeInterp& ei,
                                IDL& lengths,
                                const IDO& positions,
                                const IdsVec& ids,
                                const vtkm::Id workIndex,
                                ELL& edgelength) const
      {
        vtkm::Vec<vtkm::Id, 2> idOff;
        vtkm::Vec<vtkm::Id, 2> idLen;
        vtkm::Vec<vtkm::Id, 2> idInd;
        vtkm::Id uniqueMats = vtkm::Id(0);

        idOff[0] = vtkm::Id(0);
        idOff[1] = idOff[0];
        idInd[0] = positions.Get(ei.Vertex1);
        idInd[1] = positions.Get(ei.Vertex2);
        idLen[0] = lengths.Get(ei.Vertex1);
        idLen[1] = lengths.Get(ei.Vertex2);
        vtkm::IdComponent numberOfPoints = 2;
        vtkm::UInt8 hasWork = vtkm::UInt8(1);
        while (hasWork != vtkm::UInt8(0))
        {
          hasWork = vtkm::UInt8(0);
          vtkm::Id lowest = vtkm::Id(-1);
          for (vtkm::IdComponent i = 0; i < numberOfPoints; i++)
          {
            if (idOff[i] < idLen[i])
            {
              vtkm::Id tmp = ids.Get(idInd[i] + idOff[i]);
              if (lowest == vtkm::Id(-1) || tmp < lowest)
              {
                lowest = tmp;
                hasWork = vtkm::UInt8(1);
              }
            }
          }
          if (hasWork != vtkm::UInt8(0))
          {
            for (vtkm::IdComponent i = 0; i < numberOfPoints; i++)
            {
              if (idOff[i] < idLen[i])
              {
                vtkm::Id tmp = ids.Get(idInd[i] + idOff[i]);
                if (lowest == tmp)
                {
                  idOff[i]++;
                }
              }
            }
            uniqueMats++;
          }
        }
        lengths.Set(this->EdgePointsOffset + workIndex, uniqueMats);
        edgelength = uniqueMats;
      }
    };

    class PerformInCellInterpolations_C : public vtkm::worklet::WorkletReduceByKey
    {
    public:
      using ControlSignature = void(KeysIn keys,
                                    ValuesIn toReduce,
                                    WholeArrayIn IDLengths,
                                    WholeArrayIn IDOffsets,
                                    WholeArrayIn IDs,
                                    ReducedValuesOut centroid);

      using ExecutionSignature = void(_2, _3, _4, _5, _6);

      template <typename MappedValueVecType,
                typename MappedValueType,
                typename IDArr,
                typename IDOff,
                typename IdsVec>
      VTKM_EXEC void operator()(const MappedValueVecType& toReduce,
                                const IDArr& lengths,
                                const IDOff& positions,
                                const IdsVec& ids,
                                MappedValueType& numIdNeeded) const
      {
        vtkm::IdComponent numberOfPoints = toReduce.GetNumberOfComponents();
        // ToReduce is simply the indexArray, giving us point information (since this is reduce by key)
        // numIdNeeded is the output length of this key
        using IdVec = vtkm::Vec<vtkm::Id, 8>;
        IdVec idOff = vtkm::TypeTraits<IdVec>::ZeroInitialization();
        IdVec idLen = vtkm::TypeTraits<IdVec>::ZeroInitialization();
        IdVec idInd = vtkm::TypeTraits<IdVec>::ZeroInitialization();
        vtkm::Id uniqueMats = vtkm::Id(0);

        for (vtkm::IdComponent i = 0; i < numberOfPoints; i++)
        {
          idOff[i] = 0;
          idLen[i] = lengths.Get(toReduce[i]);
          idInd[i] = positions.Get(toReduce[i]);
        }

        vtkm::UInt8 hasWork = vtkm::UInt8(1);
        while (hasWork != vtkm::UInt8(0))
        {
          hasWork = vtkm::UInt8(0);
          vtkm::Id lowest = vtkm::Id(-1);
          for (vtkm::IdComponent i = 0; i < numberOfPoints; i++)
          {
            if (idOff[i] < idLen[i])
            {
              vtkm::Id tmp = ids.Get(idInd[i] + idOff[i]);
              if (lowest == vtkm::Id(-1) || tmp < lowest)
              {
                lowest = tmp;
                hasWork = vtkm::UInt8(1);
              }
            }
          }
          if (hasWork != vtkm::UInt8(0))
          {
            for (vtkm::IdComponent i = 0; i < numberOfPoints; i++)
            {
              if (idOff[i] < idLen[i])
              {
                vtkm::Id tmp = ids.Get(idInd[i] + idOff[i]);
                if (lowest == tmp)
                {
                  idOff[i]++;
                }
              }
            }
            uniqueMats++;
          }
        }
        numIdNeeded = uniqueMats;
      }
    };

    class PerformInCellInterpolations : public vtkm::worklet::WorkletReduceByKey
    {
    private:
      vtkm::Id offset;

    public:
      PerformInCellInterpolations(vtkm::Id outputOffsetForBookkeeping)
        : offset(outputOffsetForBookkeeping)
      {
      }
      using ControlSignature = void(KeysIn keys,
                                    ValuesIn toReduce,
                                    WholeArrayIn IDLengths,
                                    WholeArrayIn IDOffsets,
                                    WholeArrayIn IDs,
                                    WholeArrayIn VFs,
                                    ReducedValuesIn indexOff,
                                    ReducedValuesOut reindexedOut,
                                    WholeArrayOut outputIDs,
                                    WholeArrayOut outputVFs);

      using ExecutionSignature = void(_2, _3, _4, _5, _6, _7, _8, _9, _10);

      template <typename MappedValueVecType,
                typename IDArr,
                typename IDOff,
                typename IdsVec,
                typename VfsVec,
                typename IndexIn,
                typename IndexOut,
                typename OutID,
                typename OutVF>
      VTKM_EXEC void operator()(const MappedValueVecType& toReduce,
                                const IDArr& lengths,
                                const IDOff& positions,
                                const IdsVec& ids,
                                const VfsVec& vfs,
                                const IndexIn& localOffset,
                                IndexOut& globalOffset,
                                OutID& outIDs,
                                OutVF& outVFs) const
      {

        globalOffset = localOffset + this->offset;
        vtkm::IdComponent numberOfPoints = toReduce.GetNumberOfComponents();
        // ToReduce is simply the indexArray, giving us point information (since this is reduce by key)

        // numIdNeeded is the output length of this key
        using IdVec = vtkm::Vec<vtkm::Id, 8>;
        IdVec idOff = vtkm::TypeTraits<IdVec>::ZeroInitialization();
        IdVec idLen = vtkm::TypeTraits<IdVec>::ZeroInitialization();
        IdVec idInd = vtkm::TypeTraits<IdVec>::ZeroInitialization();
        vtkm::Id uniqueMats = vtkm::Id(0);

        for (vtkm::IdComponent i = 0; i < numberOfPoints; i++)
        {
          idOff[i] = 0;
          idLen[i] = lengths.Get(toReduce[i]);
          idInd[i] = positions.Get(toReduce[i]);
        }

        vtkm::UInt8 hasWork = vtkm::UInt8(1);
        while (hasWork != vtkm::UInt8(0))
        {
          hasWork = vtkm::UInt8(0);
          vtkm::Id lowest = vtkm::Id(-1);
          for (vtkm::IdComponent i = 0; i < numberOfPoints; i++)
          {
            if (idOff[i] < idLen[i])
            {
              vtkm::Id tmp = ids.Get(idInd[i] + idOff[i]);
              if (lowest == vtkm::Id(-1) || tmp < lowest)
              {
                lowest = tmp;
                hasWork = vtkm::UInt8(1);
              }
            }
          }
          if (hasWork != vtkm::UInt8(0))
          {
            vtkm::Float64 val = vtkm::Float64(0);
            for (vtkm::IdComponent i = 0; i < numberOfPoints; i++)
            {
              if (idOff[i] < idLen[i])
              {
                vtkm::Id tmp = ids.Get(idInd[i] + idOff[i]);
                if (lowest == tmp)
                {
                  val += vfs.Get(idInd[i] + idOff[i]);
                  idOff[i]++;
                }
              }
            }
            outVFs.Set(localOffset + uniqueMats, val / vtkm::Float64(numberOfPoints));
            outIDs.Set(localOffset + uniqueMats, lowest);
            uniqueMats++;
          }
        }
      }
    };

    VTKM_CONT void operator()(
      const vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& originalLen,
      const vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& originalPos,
      const vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& originalIDs,
      const vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>& originalVFs) const
    {
      vtkm::worklet::Keys<vtkm::Id> interpolationKeys(InCellInterpolationKeys);
      vtkm::Id numberOfOriginalPos = originalLen.GetNumberOfValues();
      vtkm::Id numberOfEdgePoints = EdgeInterpolationArray.GetNumberOfValues();

      vtkm::cont::ArrayHandle<vtkm::Id> lengthArr;
      vtkm::cont::ArrayHandle<vtkm::Id> posArr;
      vtkm::cont::ArrayHandle<vtkm::Id> idArr;
      vtkm::cont::ArrayHandle<vtkm::Float64> vfArr;
      lengthArr.Allocate(numberOfOriginalPos + numberOfEdgePoints);
      posArr.Allocate(numberOfOriginalPos + numberOfEdgePoints);
      vtkm::cont::Algorithm::CopySubRange(originalLen, 0, numberOfOriginalPos, lengthArr);
      vtkm::cont::Algorithm::CopySubRange(originalPos, 0, numberOfOriginalPos, posArr);

      vtkm::cont::ArrayHandle<vtkm::Id> edgeLengths;
      PerformEdgeInterpolations_C edgeCountWorklet(numberOfOriginalPos);
      vtkm::worklet::DispatcherMapField<PerformEdgeInterpolations_C> edgeInterpDispatcher_C(
        edgeCountWorklet);
      edgeInterpDispatcher_C.Invoke(
        this->EdgeInterpolationArray, lengthArr, posArr, originalIDs, edgeLengths);

      vtkm::Id idLengthFromJustEdges = vtkm::cont::Algorithm::Reduce(edgeLengths, vtkm::Id(0));

      idArr.Allocate(originalIDs.GetNumberOfValues() + idLengthFromJustEdges);
      vfArr.Allocate(originalIDs.GetNumberOfValues() + idLengthFromJustEdges);
      vtkm::cont::Algorithm::CopySubRange(originalIDs, 0, originalIDs.GetNumberOfValues(), idArr);
      vtkm::cont::Algorithm::CopySubRange(originalVFs, 0, originalIDs.GetNumberOfValues(), vfArr);
      vtkm::cont::Algorithm::ScanExclusive(lengthArr, posArr);

      // Accept that you will have to copy data :| Maybe can speed this up with some special logic...
      PerformEdgeInterpolations edgeInterpWorklet(numberOfOriginalPos);
      vtkm::worklet::DispatcherMapField<PerformEdgeInterpolations> edgeInterpDispatcher(
        edgeInterpWorklet);
      edgeInterpDispatcher.Invoke(this->EdgeInterpolationArray, lengthArr, posArr, idArr, vfArr);

      // Need to run actual edgeInterpDispatcher, we then reduce the values


      vtkm::cont::ArrayHandleIndex pointArr(numberOfOriginalPos + numberOfEdgePoints);
      vtkm::cont::ArrayHandle<vtkm::Id> pointArrCp;
      vtkm::cont::ArrayCopy(pointArr, pointArrCp);
      using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
      using ValueHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
      vtkm::cont::ArrayHandlePermutation<IdHandle, ValueHandle> toReduceValues(
        InCellInterpolationInfo, pointArrCp);

      PerformInCellInterpolations_C incellCountWorklet;
      vtkm::cont::ArrayHandle<vtkm::Id> reducedIDCounts;
      vtkm::worklet::DispatcherReduceByKey<PerformInCellInterpolations_C> cellCountDispatcher(
        incellCountWorklet);
      cellCountDispatcher.Invoke(
        interpolationKeys, toReduceValues, lengthArr, posArr, idArr, reducedIDCounts);

      vtkm::cont::ArrayHandle<vtkm::Id> reducedIDOffsets;
      vtkm::Id totalIDLen = vtkm::cont::Algorithm::ScanExclusive(reducedIDCounts, reducedIDOffsets);

      PerformInCellInterpolations incellWorklet(originalIDs.GetNumberOfValues() +
                                                idLengthFromJustEdges);
      vtkm::cont::ArrayHandle<vtkm::Id> cellids, cellOffsets;
      vtkm::cont::ArrayHandle<vtkm::Float64> cellvfs;

      cellids.Allocate(totalIDLen);
      cellvfs.Allocate(totalIDLen);
      vtkm::worklet::DispatcherReduceByKey<PerformInCellInterpolations> cellInterpDispatcher(
        incellWorklet);
      cellInterpDispatcher.Invoke(interpolationKeys,
                                  toReduceValues,
                                  lengthArr,
                                  posArr,
                                  idArr,
                                  vfArr,
                                  reducedIDOffsets,
                                  cellOffsets,
                                  cellids,
                                  cellvfs);

      vtkm::Id inCellVFOffset = originalIDs.GetNumberOfValues() + idLengthFromJustEdges;
      vtkm::cont::Algorithm::CopySubRange(cellids, 0, totalIDLen, idArr, inCellVFOffset);
      vtkm::cont::Algorithm::CopySubRange(cellvfs, 0, totalIDLen, vfArr, inCellVFOffset);
      vtkm::cont::Algorithm::CopySubRange(reducedIDCounts,
                                          0,
                                          reducedIDCounts.GetNumberOfValues(),
                                          lengthArr,
                                          numberOfOriginalPos + numberOfEdgePoints);
      vtkm::cont::Algorithm::CopySubRange(cellOffsets,
                                          0,
                                          cellOffsets.GetNumberOfValues(),
                                          posArr,
                                          numberOfOriginalPos + numberOfEdgePoints);

      *(this->LenOut) = lengthArr;
      *(this->PosOut) = posArr;
      *(this->IDOut) = idArr;
      *(this->VFOut) = vfArr;
    }

  private:
    vtkm::cont::ArrayHandle<EdgeInterpolation> EdgeInterpolationArray;
    vtkm::cont::ArrayHandle<vtkm::Id> InCellInterpolationKeys;
    vtkm::cont::ArrayHandle<vtkm::Id> InCellInterpolationInfo;
    vtkm::Id EdgePointsOffset;
    vtkm::Id InCellPointsOffset;
    IDLen* LenOut;
    IDPos* PosOut;
    IDList* IDOut;
    VFList* VFOut;
  };

  template <typename LookbackArr, typename WeightArr>
  class InterpolateLookbackField
  {
  public:
    InterpolateLookbackField(vtkm::cont::ArrayHandle<EdgeInterpolation> edgeInterpolationArray,
                             vtkm::cont::ArrayHandle<vtkm::Id> inCellInterpolationKeys,
                             vtkm::cont::ArrayHandle<vtkm::Id> inCellInterpolationInfo,
                             vtkm::Id edgePointsOffset,
                             vtkm::Id inCellPointsOffset,
                             LookbackArr* output,
                             WeightArr* output2)
      : EdgeInterpolationArray(edgeInterpolationArray)
      , InCellInterpolationKeys(inCellInterpolationKeys)
      , InCellInterpolationInfo(inCellInterpolationInfo)
      , EdgePointsOffset(edgePointsOffset)
      , InCellPointsOffset(inCellPointsOffset)
      , Output(output)
      , Output2(output2)
    {
    }
    class PerformEdgeInterpolations : public vtkm::worklet::WorkletMapField
    {
    public:
      PerformEdgeInterpolations(vtkm::Id edgePointsOffset)
        : EdgePointsOffset(edgePointsOffset)
      {
      }

      using ControlSignature = void(FieldIn edgeInterpolations,
                                    WholeArrayInOut inoutID,
                                    WholeArrayInOut inoutWeights);

      using ExecutionSignature = void(_1, _2, _3, WorkIndex);

      template <typename EdgeInterp, typename InOutId, typename InOutWeight>
      VTKM_EXEC void operator()(const EdgeInterp& ei,
                                InOutId& field,
                                InOutWeight& field1,
                                const vtkm::Id workIndex) const
      {

        vtkm::Vec<vtkm::IdComponent, 2> curOff;
        vtkm::Vec<vtkm::Float64, 2> mult;
        vtkm::Vec<vtkm::Id, 8> centroid;
        vtkm::Vec<vtkm::Float64, 8> weight;
        vtkm::Vec<vtkm::Vec<vtkm::Id, 8>, 2> keys;
        vtkm::Vec<vtkm::Vec<vtkm::Float64, 8>, 2> weights;
        keys[0] = field.Get(ei.Vertex1);
        keys[1] = field.Get(ei.Vertex2);
        weights[0] = field1.Get(ei.Vertex1);
        weights[1] = field1.Get(ei.Vertex2);
        for (vtkm::IdComponent i = 0; i < 8; i++)
        {
          weight[i] = 0;
          centroid[i] = -1;
        }
        curOff[0] = 0;
        curOff[1] = 0;
        mult[0] = ei.Weight;
        mult[1] = vtkm::Float64(1.0) - ei.Weight;
        for (vtkm::IdComponent j = 0; j < 8; j++)
        {
          vtkm::Id lowestID = vtkm::Id(-1);
          for (vtkm::IdComponent i = 0; i < 2; i++)
          {
            if (curOff[i] < 8 &&
                (lowestID == vtkm::Id(-1) ||
                 ((keys[i])[curOff[i]] != vtkm::Id(-1) && (keys[i])[curOff[i]] < lowestID)))
            {
              lowestID = (keys[i])[curOff[i]];
            }
            if (curOff[i] < 8 && (keys[i])[curOff[i]] == vtkm::Id(-1))
            {
              curOff[i] = 8;
            }
          }
          if (lowestID == vtkm::Id(-1))
          {
            break;
          }
          centroid[j] = lowestID;
          for (vtkm::IdComponent i = 0; i < 2; i++)
          {
            if (curOff[i] < 8 && lowestID == (keys[i])[curOff[i]])
            {
              weight[j] += mult[i] * weights[i][curOff[i]];
              curOff[i]++;
            }
          }
        }

        field.Set(this->EdgePointsOffset + workIndex, centroid);
        field1.Set(this->EdgePointsOffset + workIndex, weight);
      }

    private:
      vtkm::Id EdgePointsOffset;
    };

    class PerformInCellInterpolations : public vtkm::worklet::WorkletReduceByKey
    {
    public:
      using ControlSignature = void(KeysIn keys,
                                    ValuesIn toReduceID,
                                    WholeArrayIn Keys,
                                    WholeArrayIn weights,
                                    ReducedValuesOut id,
                                    ReducedValuesOut weight);

      using ExecutionSignature = void(_2, _3, _4, _5, _6);

      template <typename IDs,
                typename VecOfVecIDs,
                typename VecOfVecWeights,
                typename VecId,
                typename VecWeight>
      VTKM_EXEC void operator()(const IDs& ids,
                                const VecOfVecIDs& keysIn,
                                const VecOfVecWeights& weightsIn,
                                VecId& centroid,
                                VecWeight& weight) const
      {
        vtkm::IdComponent numValues = ids.GetNumberOfComponents();
        vtkm::Vec<vtkm::IdComponent, 8> curOff;
        vtkm::Vec<vtkm::Vec<vtkm::Id, 8>, 8> keys;
        vtkm::Vec<vtkm::Vec<vtkm::Float64, 8>, 8> weights;
        for (vtkm::IdComponent i = 0; i < 8; i++)
        {
          weight[i] = 0;
          centroid[i] = -1;
          curOff[i] = 0;
        }
        for (vtkm::IdComponent i = 0; i < numValues; i++)
        {
          keys[i] = keysIn.Get(ids[i]);
          weights[i] = weightsIn.Get(ids[i]);
        }
        for (vtkm::IdComponent i = numValues; i < 8; i++)
        {
          curOff[i] = 8;
        }
        for (vtkm::IdComponent j = 0; j < 8; j++)
        {
          vtkm::Id lowestID = vtkm::Id(-1);
          for (vtkm::IdComponent i = 0; i < numValues; i++)
          {
            auto tmp = keys[i];
            if (curOff[i] < 8 &&
                (lowestID == vtkm::Id(-1) ||
                 (tmp[curOff[i]] != vtkm::Id(-1) && tmp[curOff[i]] < lowestID)))
            {
              lowestID = tmp[curOff[i]];
            }

            if (curOff[i] < 8 && tmp[curOff[i]] == vtkm::Id(-1))
            {
              curOff[i] = 8;
            }
          }
          if (lowestID == vtkm::Id(-1))
          {
            break;
          }
          centroid[j] = lowestID;
          for (vtkm::IdComponent i = 0; i < numValues; i++)
          {
            auto tmp = keys[i];
            if (curOff[i] < 8 && lowestID == tmp[curOff[i]])
            {
              auto w = weights[i];
              weight[j] += w[curOff[i]];
              curOff[i]++;
            }
          }
        }
        for (vtkm::IdComponent j = 0; j < 8; j++)
        {
          weight[j] *= 1. / static_cast<vtkm::Float64>(numValues);
          VTKM_ASSERT(curOff[j] == 8);
        }
      }
    };

    template <typename ValueType, typename ValueType1, typename Storage, typename Storage2>
    VTKM_CONT void operator()(
      const vtkm::cont::ArrayHandle<ValueType, Storage>& fieldID,
      const vtkm::cont::ArrayHandle<ValueType1, Storage2>& weightsField) const
    {
      vtkm::worklet::Keys<vtkm::Id> interpolationKeys(InCellInterpolationKeys);

      vtkm::Id numberOfOriginalValues = fieldID.GetNumberOfValues();
      vtkm::Id numberOfEdgePoints = EdgeInterpolationArray.GetNumberOfValues();
      vtkm::Id numberOfInCellPoints = interpolationKeys.GetUniqueKeys().GetNumberOfValues();
      LookbackArr result;
      result.Allocate(numberOfOriginalValues + numberOfEdgePoints + numberOfInCellPoints);
      vtkm::cont::Algorithm::CopySubRange(fieldID, 0, numberOfOriginalValues, result);
      WeightArr result2;
      result2.Allocate(numberOfOriginalValues + numberOfEdgePoints + numberOfInCellPoints);
      vtkm::cont::Algorithm::CopySubRange(weightsField, 0, numberOfOriginalValues, result2);

      PerformEdgeInterpolations edgeInterpWorklet(numberOfOriginalValues);
      vtkm::worklet::DispatcherMapField<PerformEdgeInterpolations> edgeInterpDispatcher(
        edgeInterpWorklet);
      edgeInterpDispatcher.Invoke(this->EdgeInterpolationArray, result, result2);

      // Perform a gather on output to get all required values for calculation of
      // centroids using the interpolation info array.oi
      vtkm::cont::ArrayHandleIndex nout(numberOfOriginalValues + numberOfEdgePoints);
      auto toReduceValues = make_ArrayHandlePermutation(InCellInterpolationInfo, nout);

      vtkm::cont::ArrayHandle<ValueType> reducedValues;
      vtkm::cont::ArrayHandle<ValueType1> reducedWeights;
      vtkm::worklet::DispatcherReduceByKey<PerformInCellInterpolations>
        inCellInterpolationDispatcher;
      inCellInterpolationDispatcher.Invoke(
        interpolationKeys, toReduceValues, result, result2, reducedValues, reducedWeights);
      vtkm::Id inCellPointsOffset = numberOfOriginalValues + numberOfEdgePoints;
      vtkm::cont::Algorithm::CopySubRange(
        reducedValues, 0, reducedValues.GetNumberOfValues(), result, inCellPointsOffset);
      vtkm::cont::Algorithm::CopySubRange(
        reducedWeights, 0, reducedWeights.GetNumberOfValues(), result2, inCellPointsOffset);
      *(this->Output) = result;
      *(this->Output2) = result2;
    }

  private:
    vtkm::cont::ArrayHandle<EdgeInterpolation> EdgeInterpolationArray;
    vtkm::cont::ArrayHandle<vtkm::Id> InCellInterpolationKeys;
    vtkm::cont::ArrayHandle<vtkm::Id> InCellInterpolationInfo;
    vtkm::Id EdgePointsOffset;
    vtkm::Id InCellPointsOffset;
    LookbackArr* Output;
    WeightArr* Output2;
  };
  void ProcessSimpleMIRField(
    const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 8>, vtkm::cont::StorageTagBasic>& orLookback,
    const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 8>, vtkm::cont::StorageTagBasic>&
      orWeights,
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 8>, vtkm::cont::StorageTagBasic>& newLookback,
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 8>, vtkm::cont::StorageTagBasic>& newweights)
    const
  {
    auto worker = InterpolateLookbackField<vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 8>>,
                                           vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 8>>>(
      this->EdgePointsInterpolation,
      this->InCellInterpolationKeys,
      this->InCellInterpolationInfo,
      this->EdgePointsOffset,
      this->InCellPointsOffset,
      &newLookback,
      &newweights);
    worker(orLookback, orWeights);
  }
  void ProcessMIRField(
    const vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> orLen,
    const vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> orPos,
    const vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> orIDs,
    const vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic> orVFs,
    vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& newLen,
    vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& newPos,
    vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& newIDs,
    vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>& newVFs) const
  {
    auto worker =
      InterpolateMIRFields<vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>,
                           vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>,
                           vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>,
                           vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>>(
        this->EdgePointsInterpolation,
        this->InCellInterpolationKeys,
        this->InCellInterpolationInfo,
        this->EdgePointsOffset,
        this->InCellPointsOffset,
        &newLen,
        &newPos,
        &newIDs,
        &newVFs);
    worker(orLen, orPos, orIDs, orVFs);
  }

  template <typename ValueType, typename StorageType>
  vtkm::cont::ArrayHandle<ValueType> ProcessPointField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& fieldData) const
  {
    using ResultType = vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagBasic>;
    using Worker = InterpolateField<ResultType>;
    ResultType output;
    Worker worker = Worker(this->EdgePointsInterpolation,
                           this->InCellInterpolationKeys,
                           this->InCellInterpolationInfo,
                           this->EdgePointsOffset,
                           this->InCellPointsOffset,
                           &output);
    worker(fieldData);
    return output;
  }




  template <typename ValueType, typename StorageType>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& fieldData) const
  {
    // Use a temporary permutation array to simplify the mapping:
    auto tmp = vtkm::cont::make_ArrayHandlePermutation(this->CellMapOutputToInput, fieldData);

    // Copy into an array with default storage:
    vtkm::cont::ArrayHandle<ValueType> result;
    vtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

private:
  MIRCases::MIRTables MIRTablesInstance;
  vtkm::cont::ArrayHandle<EdgeInterpolation> EdgePointsInterpolation;
  vtkm::cont::ArrayHandle<vtkm::Id> InCellInterpolationKeys;
  vtkm::cont::ArrayHandle<vtkm::Id> InCellInterpolationInfo;
  vtkm::cont::ArrayHandle<vtkm::Id> CellMapOutputToInput;
  vtkm::Id EdgePointsOffset;
  vtkm::Id InCellPointsOffset;
};

}
}

namespace vtkm
{
namespace worklet
{
template <typename IDType, typename FloatType>
struct MIRObject : public vtkm::cont::ExecutionAndControlObjectBase
{
public:
  class MIRObjectPortal
  {
  public:
    VTKM_EXEC FloatType GetVFForPoint(IDType point, IDType matID, IDType) const
    {
      IDType low = this->PPos.Get(point);
      IDType high = this->PPos.Get(point) + this->PLens.Get(point) - 1;
      IDType matIdAt = -1;
      while (low <= high)
      {
        IDType mid = (low + high) / 2;
        IDType midMatId = this->PIDs.Get(mid);
        if (matID == midMatId)
        {
          matIdAt = mid;
          break;
        }
        else if (matID > midMatId)
        {
          low = mid + 1;
        }
        else if (matID < midMatId)
        {
          high = mid - 1;
        }
      }
      if (matIdAt >= 0)
      {
        return this->PVFs.Get(matIdAt);
      }
      else
        return FloatType(0);
    }

  private:
    typename vtkm::cont::ArrayHandle<IDType, vtkm::cont::StorageTagBasic>::ReadPortalType PLens;
    typename vtkm::cont::ArrayHandle<IDType, vtkm::cont::StorageTagBasic>::ReadPortalType PPos;
    typename vtkm::cont::ArrayHandle<IDType, vtkm::cont::StorageTagBasic>::ReadPortalType PIDs;
    typename vtkm::cont::ArrayHandle<FloatType, vtkm::cont::StorageTagBasic>::ReadPortalType PVFs;
    friend struct MIRObject;
  };

  VTKM_CONT vtkm::cont::ArrayHandle<IDType> getPointLenArr() { return this->pointLen; }
  VTKM_CONT vtkm::cont::ArrayHandle<IDType> getPointPosArr() { return this->pointPos; }
  VTKM_CONT vtkm::cont::ArrayHandle<IDType> getPointIDArr() { return this->pointIDs; }
  VTKM_CONT vtkm::cont::ArrayHandle<FloatType> getPointVFArr() { return this->pointVFs; }

  // Do we need to copy these arrays?
  template <typename IDInput, typename FloatInput>
  MIRObject(const IDInput& len, const IDInput& pos, const IDInput& ids, const FloatInput& floats)
    : pointLen(len)
    , pointPos(pos)
    , pointIDs(ids)
    , pointVFs(floats)
  {
  }

  MIRObjectPortal PrepareForExecution(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token& token)
  {
    MIRObjectPortal portal;
    portal.PLens = this->pointLen.PrepareForInput(device, token);
    portal.PPos = this->pointPos.PrepareForInput(device, token);
    portal.PIDs = this->pointIDs.PrepareForInput(device, token);
    portal.PVFs = this->pointVFs.PrepareForInput(device, token);
    return portal;
  }

private:
  vtkm::cont::ArrayHandle<IDType> pointLen, pointPos, pointIDs;
  vtkm::cont::ArrayHandle<FloatType> pointVFs;
};

struct CombineVFsForPoints_C : public vtkm::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInCell lens,
                                FieldInCell pos,
                                WholeArrayIn ids,
                                FieldOutPoint idcount);
  using ExecutionSignature = void(CellCount, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename LenVec, typename PosVec, typename IdsVec, typename OutVec>
  VTKM_EXEC void operator()(vtkm::IdComponent numCells,
                            const LenVec& len,
                            const PosVec& pos,
                            const IdsVec& ids,
                            OutVec& outlength) const
  {

    // This is for the number of VFs in the surrounding cells...
    // We assume that the ids are sorted.
    outlength = vtkm::Id(0);

    vtkm::Id uniqueMats = vtkm::Id(0);

    using ida = vtkm::Id;

    ida lowest = ids.Get(pos[0]);
    ida prevLowest = ida(-1);
    ida largest = ida(-1);

    for (vtkm::IdComponent ci = 0; ci < numCells; ci++)
    {
      vtkm::IdComponent l = vtkm::IdComponent(pos[ci] + len[ci]);
      for (vtkm::IdComponent idi = vtkm::IdComponent(pos[ci]); idi < l; idi++)
      {
        ida tmp = ids.Get(idi);
        largest = vtkm::Maximum()(tmp, largest);
      }
    }

    while (prevLowest != lowest)
    {
      for (vtkm::IdComponent ci = 0; ci < numCells; ci++)
      {
        vtkm::IdComponent l = vtkm::IdComponent(pos[ci] + len[ci]);
        for (vtkm::IdComponent idi = vtkm::IdComponent(pos[ci]); idi < l; idi++)
        {
          ida tmp = ids.Get(idi);
          if (tmp < lowest && tmp > prevLowest)
          {
            lowest = tmp;
          }
        }
      }
      uniqueMats++;
      prevLowest = ida(lowest);
      lowest = ida(largest);
    }
    outlength = uniqueMats;
  }
};

struct CombineVFsForPoints : public vtkm::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInCell lens,
                                FieldInCell pos,
                                WholeArrayIn ids,
                                WholeArrayIn vfs,
                                FieldInPoint actpos,
                                WholeArrayOut idx,
                                WholeArrayOut vfx);
  using ExecutionSignature = void(CellCount, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;
  template <typename LenVec,
            typename PosVec,
            typename IdsVec,
            typename VfsVec,
            typename PosVec2,
            typename OutVec,
            typename OutVec2>
  VTKM_EXEC void operator()(vtkm::IdComponent numCells,
                            const LenVec& len,
                            const PosVec& pos,
                            const IdsVec& ids,
                            const VfsVec& vfs,
                            const PosVec2& posit,
                            OutVec& outid,
                            OutVec2& outvf) const
  {

    // This is for the number of VFs in the surrounding cells...
    // We assume that the ids are sorted.


    vtkm::Id uniqueMats = vtkm::Id(0);

    using ida = vtkm::Id;

    ida lowest = ids.Get(pos[0]);
    ida prevLowest = ida(-1);
    ida largest = ida(-1);

    for (vtkm::IdComponent ci = 0; ci < numCells; ci++)
    {
      vtkm::IdComponent l = vtkm::IdComponent(pos[ci] + len[ci]);
      for (vtkm::IdComponent idi = vtkm::IdComponent(pos[ci]); idi < l; idi++)
      {
        ida tmp = ids.Get(idi);
        largest = vtkm::Maximum()(tmp, largest);
      }
    }

    while (prevLowest != lowest)
    {
      for (vtkm::IdComponent ci = 0; ci < numCells; ci++)
      {
        vtkm::IdComponent l = vtkm::IdComponent(pos[ci] + len[ci]);
        for (vtkm::IdComponent idi = vtkm::IdComponent(pos[ci]); idi < l; idi++)
        {
          ida tmp = ids.Get(idi);
          if (tmp < lowest && tmp > prevLowest)
          {
            lowest = tmp;
          }
        }
      }
      outid.Set(posit + uniqueMats, lowest);
      vtkm::Float64 avg = vtkm::Float64(0);
      for (vtkm::IdComponent ci = 0; ci < numCells; ci++)
      {
        vtkm::IdComponent l = vtkm::IdComponent(pos[ci] + len[ci]);
        for (vtkm::IdComponent idi = vtkm::IdComponent(pos[ci]); idi < l; idi++)
        {
          ida tmp = ids.Get(idi);
          if (tmp == lowest)
          {
            avg += vfs.Get(idi);
          }
        }
      }
      outvf.Set(posit + uniqueMats, avg / vtkm::Float64(numCells));
      uniqueMats++;
      prevLowest = ida(lowest);
      lowest = ida(largest);
    }
  }
};

struct ExtractVFsForMIR_C : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet, FieldOutCell numPointsCount);
  using ExecutionSignature = void(PointCount, _2);
  using InputDomain = _1;
  template <typename OutVec>
  VTKM_EXEC void operator()(vtkm::IdComponent numPoints, OutVec& outlength) const
  {
    outlength = numPoints;
  }
};

struct ExtractVFsForMIR : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                ExecObject mir_obj,
                                FieldInCell previousMatID,
                                FieldOutCell curvfVals,
                                FieldOutCell prevvfVals);

  using ExecutionSignature = void(PointCount, VisitIndex, PointIndices, _2, _3, _4, _5);
  using InputDomain = _1;
  using ScatterType = vtkm::worklet::ScatterCounting;
  template <typename CountArrayType>
  VTKM_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
  {
    VTKM_IS_ARRAY_HANDLE(CountArrayType);
    return ScatterType(countArray);
  }
  template <typename DA, typename prevID, typename OutVec, typename OutVec2, typename pointVec>
  VTKM_EXEC void operator()(vtkm::IdComponent numPoints,
                            vtkm::IdComponent index,
                            pointVec& pointIDs,
                            const DA& mirobj,
                            const prevID& previousID,
                            OutVec& outVF,
                            OutVec2& prevOutVF) const
  {
    (void)numPoints;
    outVF = OutVec(0);
    prevOutVF = OutVec2(0);
    outVF = mirobj.GetVFForPoint(pointIDs[index], this->target, 0);
    if (previousID == 0)
    {
      prevOutVF = 0;
    }
    else
    {
      prevOutVF = mirobj.GetVFForPoint(pointIDs[index], previousID, 0);
    }
  }
  ExtractVFsForMIR(vtkm::Id targetMat)
    : target(targetMat)
  {
  }

private:
  vtkm::Id target;
};

struct CalcVol : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                ExecObject mirTables,
                                FieldInPoint verts,
                                FieldOutCell vol);
  using ExecutionSignature = void(PointCount, CellShape, _2, _3, _4);
  template <typename Arrout, typename PointListIn, typename Dev, typename CellShape>
  VTKM_EXEC void operator()(const vtkm::IdComponent pointCount,
                            const CellShape& cellShape,
                            const Dev& mirTable,
                            const PointListIn& vertPos,
                            Arrout& volumeOut) const
  {
    vtkm::IdComponent numFaces = mirTable.GetNumberOfFaces(static_cast<vtkm::Id>(cellShape.Id));

    vtkm::Float64 totVol = vtkm::Float64(0);
    vtkm::IdComponent offset = mirTable.GetFaceOffset(static_cast<vtkm::Id>(cellShape.Id));

    auto av1 = vertPos[0];
    for (vtkm::IdComponent i = 1; i < pointCount; i++)
    {
      av1 += vertPos[i];
    }
    auto av = av1 * (vtkm::Float64(1.0) / vtkm::Float64(pointCount));

    for (vtkm::IdComponent i = 0; i < numFaces; i++)
    {
      vtkm::UInt8 p1 = mirTable.GetPoint(offset++);
      vtkm::UInt8 p2 = mirTable.GetPoint(offset++);
      vtkm::UInt8 p3 = mirTable.GetPoint(offset++);
      auto v1 = vertPos[p1];
      auto v2 = vertPos[p2];
      auto v3 = vertPos[p3];

      auto v4 = v1 - av;
      auto v5 = v2 - av;
      auto v6 = v3 - av;
      totVol += vtkm::Abs(vtkm::Dot(v4, vtkm::Cross(v5, v6))) / 6;
    }
    volumeOut = totVol;
  }
};

struct CalcError_C : public vtkm::worklet::WorkletReduceByKey
{
public:
  using ControlSignature = void(KeysIn cellID,
                                ValuesIn cellColors,
                                WholeArrayIn origLen,
                                WholeArrayIn origPos,
                                WholeArrayIn origIDs,
                                WholeArrayOut newlengthsOut);
  using ExecutionSignature = void(ValueCount, _1, _2, _3, _4, _5, _6);
  using InputDomain = _1;
  template <typename Colors, typename ORL, typename ORP, typename ORID, typename NLO>
  VTKM_EXEC void operator()(const vtkm::IdComponent numCells,
                            const vtkm::Id cellID,
                            const Colors& cellCol,
                            const ORL& orgLen,
                            const ORP& orgPos,
                            const ORID& orgID,
                            NLO& outputLen) const
  {
    // Although I don't doubt for a minute that keys is sorted and hence the output would be too,
    // but this ensures I don't deal with a headache if they change that.
    // The orgLen and orgPos are the true, original cell IDs and VFs
    // Luckily indexing into cellID should be quick compared to orgLen...
    vtkm::Id lowest = orgID.Get(orgPos.Get(0));
    vtkm::Id originalInd = 0;
    vtkm::Id orgLen1 = orgLen.Get(cellID);
    vtkm::Id orgPos1 = orgPos.Get(cellID);
    vtkm::Id uniqueMats = 0;
    vtkm::Id largest = orgID.Get(orgLen1 + orgPos1 - 1);
    for (vtkm::IdComponent i = 0; i < numCells; i++)
    {
      vtkm::Id tmp = cellCol[i];
      largest = vtkm::Maximum()(tmp, largest);
    }
    vtkm::Id prevLowest = vtkm::Id(-1);
    lowest = vtkm::Id(0);
    while (prevLowest != largest)
    {
      if (originalInd < orgLen1)
      {
        lowest = orgID.Get(orgPos1 + originalInd);
      }
      for (vtkm::IdComponent i = 0; i < numCells; i++)
      {
        vtkm::Id tmp = cellCol[i];
        if (tmp > prevLowest)
        {
          lowest = vtkm::Minimum()(tmp, lowest);
        }
      }
      if (originalInd < orgLen1)
      {
        if (orgID.Get(orgPos1 + originalInd) == lowest)
        {
          originalInd++;
        }
      }
      uniqueMats++;

      prevLowest = lowest;
      lowest = largest;
    }
    outputLen.Set(cellID, uniqueMats);
  }
};

struct CalcError : public vtkm::worklet::WorkletReduceByKey
{
private:
  vtkm::Float64 lerping;

public:
  CalcError(vtkm::Float64 errorLerp)
    : lerping(errorLerp)
  {
  }
  using ControlSignature = void(KeysIn cellID,
                                ValuesIn cellColors,
                                ValuesIn cellVols,
                                WholeArrayIn origLen,
                                WholeArrayIn origPos,
                                WholeArrayIn origIDs,
                                WholeArrayIn origVFs,
                                WholeArrayIn curLen,
                                WholeArrayIn curPos,
                                WholeArrayIn curID,
                                WholeArrayIn curVF,
                                WholeArrayIn newLength,
                                WholeArrayIn newposition,
                                WholeArrayOut newIDs,
                                WholeArrayOut newVFs,
                                WholeArrayIn origVols,
                                ReducedValuesOut totalErr);
  using ExecutionSignature =
    void(ValueCount, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17);
  using InputDomain = _1;
  template <typename Colors,
            typename ORL,
            typename ORP,
            typename ORID,
            typename NLO,
            typename ORVF,
            typename NID,
            typename NVF,
            typename Vols,
            typename TEO,
            typename CPos,
            typename CLen,
            typename CID,
            typename CVF,
            typename NLen,
            typename OVols>
  VTKM_EXEC void operator()(const vtkm::IdComponent numCells,
                            const vtkm::Id cellID,
                            const Colors& cellCol,
                            const Vols& cellVolumes,
                            const ORL& orgLen,
                            const ORP& orgPos,
                            const ORID& orgID,
                            const ORVF& orgVF,
                            const CLen& curLen,
                            const CPos& curPos,
                            const CID& curID,
                            const CVF& curVF,
                            const NLen&,
                            const NLO& inputPos,
                            NID& inputIDs,
                            NVF& inputVFs,
                            const OVols& orgVols,
                            TEO& totalErrorOut) const
  {
    // Although I don't doubt for a minute that keys is sorted and hence the output would be too,
    // but this ensures I don't deal with a headache if they change that.
    // The orgLen and orgPos are the true, original cell IDs and VFs
    // Luckily indexing into cellID should be quick compared to orgLen...
    //{
    vtkm::Id lowest = orgID.Get(orgPos.Get(0));
    vtkm::Id originalInd = 0;
    vtkm::Id orgLen1 = orgLen.Get(cellID);
    vtkm::Id orgPos1 = orgPos.Get(cellID);
    vtkm::Id uniqueMats = 0;
    vtkm::Id largest = orgID.Get(orgLen1 + orgPos1 - 1);

    //vtkm::Id canConnect = vtkm::Id(0);
    for (vtkm::IdComponent i = 0; i < numCells; i++)
    {
      vtkm::Id tmp = cellCol[i];
      largest = vtkm::Maximum()(tmp, largest);
    }
    vtkm::Id prevLowest = vtkm::Id(-1);

    vtkm::Id currentIndex = curPos.Get(cellID);
    vtkm::Id currentLens = curLen.Get(cellID) + currentIndex;
    //}

    vtkm::Float64 totalError = vtkm::Float64(0);
    while (prevLowest != largest)
    {
      if (originalInd < orgLen1)
      {
        lowest = orgID.Get(orgPos1 + originalInd);
      }
      for (vtkm::IdComponent i = 0; i < numCells; i++)
      {
        vtkm::Id tmp = cellCol[i];
        if (tmp > prevLowest)
        {
          lowest = vtkm::Minimum()(tmp, lowest);
        }
      }
      vtkm::Float64 totalVolForColor = vtkm::Float64(0);
      for (vtkm::IdComponent i = 0; i < numCells; i++)
      {
        vtkm::Id tmp = cellCol[i];
        if (tmp == lowest)
        {
          totalVolForColor += cellVolumes[i];
        }
      }
      if (originalInd < orgLen1)
      {
        if (orgID.Get(orgPos1 + originalInd) == lowest)
        {
          totalVolForColor -= orgVF.Get(orgPos1 + originalInd) * orgVols.Get(cellID);
          originalInd++;
        }
      }

      vtkm::Float64 prevTarget = vtkm::Float64(0);
      if (currentIndex < currentLens)
      {

        if (curID.Get(currentIndex) == lowest)
        {
          prevTarget = curVF.Get(currentIndex);
          currentIndex++;
        }
      }
      //vtkm::Float64 tmp = prevTarget;
      prevTarget += this->lerping * (-totalVolForColor) / orgVols.Get(cellID);
      totalError += vtkm::Abs(totalVolForColor);
      //VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "Lerping " << tmp << " -> " << prevTarget << " :| " << totalVolForColor);
      //VTKM_LOG_S(vtkm::cont::LogLevel::Info, cellID << ": " << uniqueMats << " PT: " << tmp << " AVPTR "
      //            << totalVolForColor << " L: " << this->lerping << " and " << prevTarget << " / " << totalError
      //            << "\n" << inputPos.Get(cellID));
      inputIDs.Set(inputPos.Get(cellID) + uniqueMats, lowest);
      inputVFs.Set(inputPos.Get(cellID) + uniqueMats, vtkm::FloatDefault(prevTarget));
      uniqueMats++;

      prevLowest = lowest;
      lowest = largest;
    }
    totalErrorOut = TEO(totalError);
  }
};
struct CheckFor2D : public vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn,
                                FieldOutCell is2D,
                                FieldOutCell is3D,
                                FieldOutCell isOther);
  using ExecutionSignature = void(CellShape, _2, _3, _4);
  using InputDomain = _1;
  template <typename OO, typename OP, typename OQ, typename SHAPE>
  VTKM_EXEC void operator()(const SHAPE shape, OO& is2D, OP& is3D, OQ& isOther) const
  {
    is2D = vtkm::Id(0);
    is3D = vtkm::Id(0);

    if (shape.Id == vtkm::CellShapeIdToTag<vtkm::CELL_SHAPE_TRIANGLE>::Tag().Id ||
        shape.Id == vtkm::CellShapeIdToTag<vtkm::CELL_SHAPE_POLYGON>::Tag().Id ||
        shape.Id == vtkm::CellShapeIdToTag<vtkm::CELL_SHAPE_QUAD>::Tag().Id ||
        shape.Id == vtkm::Id(6) // Tri strip?
        || shape.Id == vtkm::Id(8) /* Pixel? */)
    {
      is2D = vtkm::Id(1);
    }
    else if (shape.Id == vtkm::Id(0) /*Empty*/
             || shape.Id == vtkm::CellShapeIdToTag<vtkm::CELL_SHAPE_LINE>::Tag().Id ||
             shape.Id == vtkm::CellShapeIdToTag<vtkm::CELL_SHAPE_POLY_LINE>::Tag().Id ||
             shape.Id == vtkm::CellShapeIdToTag<vtkm::CELL_SHAPE_VERTEX>::Tag().Id ||
             shape.Id == vtkm::Id(2) /* Poly Vertex? */)
    {
      isOther = vtkm::Id(1);
    }
    else if (shape.Id == vtkm::CellShapeIdToTag<vtkm::CELL_SHAPE_TETRA>::Tag().Id ||
             shape.Id == vtkm::CellShapeIdToTag<vtkm::CELL_SHAPE_HEXAHEDRON>::Tag().Id ||
             shape.Id == vtkm::CellShapeIdToTag<vtkm::CELL_SHAPE_WEDGE>::Tag().Id ||
             shape.Id == vtkm::CellShapeIdToTag<vtkm::CELL_SHAPE_PYRAMID>::Tag().Id ||
             shape.Id == vtkm::Id(11) /* Voxel? */)
    {
      is3D = vtkm::Id(1);
    }
    else
    {
      // Truly is other
      isOther = vtkm::Id(1);
    }
  }
};

struct ConstructCellWeightList : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn pointIDs, FieldOut VecLookback, FieldOut VecWeights);
  using ExecutionSignature = void(InputIndex, _2, _3);
  using InputDomain = _1;
  template <typename VO1, typename VO2>
  VTKM_EXEC void operator()(vtkm::Id& in, VO1& lookback, VO2& weights) const
  {
    for (vtkm::IdComponent i = 0; i < 8; i++)
    {
      lookback[i] = vtkm::Id(-1);
      weights[i] = vtkm::Float64(0);
    }
    lookback[0] = in;
    weights[0] = vtkm::Float64(1);
  }
};

struct DestructPointWeightList : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn pointIDs,
                                FieldIn pointWeights,
                                WholeArrayIn originalVals,
                                FieldOut newVals);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;
  template <typename PID, typename PW, typename OV, typename NV>
  VTKM_EXEC void operator()(const PID& pids, const PW& pws, const OV& ov, NV& newVals) const
  {
    VTKM_ASSERT(pids[0] != -1);
    newVals = static_cast<NV>(ov.Get(pids[0]) * pws[0]);
    for (vtkm::IdComponent i = 1; i < 8; i++)
    {
      if (pids[i] == vtkm::Id(-1))
      {
        break;
      }
      else
      {
        newVals += static_cast<NV>(ov.Get(pids[i]) * pws[i]);
      }
    }
  }
};

}

}

#endif
