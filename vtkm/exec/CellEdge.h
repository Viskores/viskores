//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_CellEdge_h
#define vtk_m_exec_CellEdge_h

#include <vtkm/Assert.h>
#include <vtkm/CellShape.h>
#include <vtkm/CellTraits.h>
#include <vtkm/Types.h>
#include <vtkm/exec/FunctorBase.h>
#include <vtkm/internal/Assume.h>

namespace vtkm
{
namespace exec
{

namespace detail
{

class CellEdgeTables
{
public:
  static constexpr vtkm::Int32 MAX_NUM_EDGES = 12;

public:
  VTKM_EXEC vtkm::Int32 NumEdges(vtkm::Int32 cellShapeId) const
  {
    VTKM_STATIC_CONSTEXPR_ARRAY vtkm::Int32 numEdges[vtkm::NUMBER_OF_CELL_SHAPES] = {
      // NumEdges
      0,  //  0: CELL_SHAPE_EMPTY
      0,  //  1: CELL_SHAPE_VERTEX
      0,  //  2: Unused
      0,  //  3: CELL_SHAPE_LINE
      0,  //  4: CELL_SHAPE_POLY_LINE
      3,  //  5: CELL_SHAPE_TRIANGLE
      0,  //  6: Unused
      -1, //  7: CELL_SHAPE_POLYGON  ---special case---
      0,  //  8: Unused
      4,  //  9: CELL_SHAPE_QUAD
      6,  // 10: CELL_SHAPE_TETRA
      0,  // 11: Unused
      12, // 12: CELL_SHAPE_HEXAHEDRON
      9,  // 13: CELL_SHAPE_WEDGE
      8   // 14: CELL_SHAPE_PYRAMID
    };
    return numEdges[cellShapeId];
  }

  VTKM_EXEC vtkm::Int32 PointsInEdge(vtkm::Int32 cellShapeId,
                                     vtkm::Int32 edgeIndex,
                                     vtkm::Int32 localPointIndex) const
  {
    VTKM_STATIC_CONSTEXPR_ARRAY vtkm::Int32
      pointsInEdge[vtkm::NUMBER_OF_CELL_SHAPES][MAX_NUM_EDGES][2] = {
        // clang-format off
        //  0: CELL_SHAPE_EMPTY
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  1: CELL_SHAPE_VERTEX
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  2: Unused
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  3: CELL_SHAPE_LINE
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  4: CELL_SHAPE_POLY_LINE
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  5: CELL_SHAPE_TRIANGLE
        { { 0, 1 },   { 1, 2 },   { 2, 0 },   { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  6: Unused
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  7: CELL_SHAPE_POLYGON  --- special case ---
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  8: Unused
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  9: CELL_SHAPE_QUAD
        { { 0, 1 },   { 1, 2 },   { 2, 3 },   { 3, 0 },   { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        // 10: CELL_SHAPE_TETRA
        { { 0, 1 },   { 1, 2 },   { 2, 0 },   { 0, 3 },   { 1, 3 },   { 2, 3 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        // 11: Unused
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        // 12: CELL_SHAPE_HEXAHEDRON
        { { 0, 1 }, { 1, 2 }, { 3, 2 }, { 0, 3 }, { 4, 5 }, { 5, 6 },
          { 7, 6 }, { 4, 7 }, { 0, 4 }, { 1, 5 }, { 3, 7 }, { 2, 6 } },
        // 13: CELL_SHAPE_WEDGE
        { { 0, 1 }, { 1, 2 }, { 2, 0 }, { 3, 4 },   { 4, 5 },   { 5, 3 },
          { 0, 3 }, { 1, 4 }, { 2, 5 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        // 14: CELL_SHAPE_PYRAMID
        { { 0, 1 }, { 1, 2 }, { 2, 3 },   { 3, 0 },   { 0, 4 },   { 1, 4 },
          { 2, 4 }, { 3, 4 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } }
        // clang-format on
      };

    return pointsInEdge[cellShapeId][edgeIndex][localPointIndex];
  }
};

} // namespace detail

template <typename CellShapeTag>
static inline VTKM_EXEC vtkm::IdComponent CellEdgeNumberOfEdges(vtkm::IdComponent numPoints,
                                                                CellShapeTag,
                                                                const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == vtkm::CellTraits<CellShapeTag>::NUM_POINTS);
  return detail::CellEdgeTables{}.NumEdges(CellShapeTag::Id);
}

static inline VTKM_EXEC vtkm::IdComponent CellEdgeNumberOfEdges(vtkm::IdComponent numPoints,
                                                                vtkm::CellShapeTagPolygon,
                                                                const vtkm::exec::FunctorBase&)
{
  VTKM_ASSUME(numPoints > 0);
  return numPoints;
}

static inline VTKM_EXEC vtkm::IdComponent CellEdgeNumberOfEdges(vtkm::IdComponent numPoints,
                                                                vtkm::CellShapeTagPolyLine,
                                                                const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSUME(numPoints > 0);
  return detail::CellEdgeTables{}.NumEdges(vtkm::CELL_SHAPE_POLY_LINE);
}

static inline VTKM_EXEC vtkm::IdComponent CellEdgeNumberOfEdges(
  vtkm::IdComponent numPoints,
  vtkm::CellShapeTagGeneric shape,
  const vtkm::exec::FunctorBase& worklet)
{
  if (shape.Id == vtkm::CELL_SHAPE_POLYGON)
  {
    return CellEdgeNumberOfEdges(numPoints, vtkm::CellShapeTagPolygon(), worklet);
  }
  else if (shape.Id == vtkm::CELL_SHAPE_POLY_LINE)
  {
    return CellEdgeNumberOfEdges(numPoints, vtkm::CellShapeTagPolyLine(), worklet);
  }
  else
  {
    return detail::CellEdgeTables{}.NumEdges(shape.Id);
  }
}

template <typename CellShapeTag>
static inline VTKM_EXEC vtkm::IdComponent CellEdgeLocalIndex(vtkm::IdComponent numPoints,
                                                             vtkm::IdComponent pointIndex,
                                                             vtkm::IdComponent edgeIndex,
                                                             CellShapeTag shape,
                                                             const vtkm::exec::FunctorBase& worklet)
{
  VTKM_ASSUME(pointIndex >= 0);
  VTKM_ASSUME(pointIndex < 2);
  VTKM_ASSUME(edgeIndex >= 0);
  VTKM_ASSUME(edgeIndex < detail::CellEdgeTables::MAX_NUM_EDGES);
  if (edgeIndex >= vtkm::exec::CellEdgeNumberOfEdges(numPoints, shape, worklet))
  {
    worklet.RaiseError("Invalid edge number.");
    return 0;
  }

  detail::CellEdgeTables table;
  return table.PointsInEdge(CellShapeTag::Id, edgeIndex, pointIndex);
}

static inline VTKM_EXEC vtkm::IdComponent CellEdgeLocalIndex(vtkm::IdComponent numPoints,
                                                             vtkm::IdComponent pointIndex,
                                                             vtkm::IdComponent edgeIndex,
                                                             vtkm::CellShapeTagPolygon,
                                                             const vtkm::exec::FunctorBase&)
{
  VTKM_ASSUME(numPoints >= 3);
  VTKM_ASSUME(pointIndex >= 0);
  VTKM_ASSUME(pointIndex < 2);
  VTKM_ASSUME(edgeIndex >= 0);
  VTKM_ASSUME(edgeIndex < numPoints);

  if (edgeIndex + pointIndex < numPoints)
  {
    return edgeIndex + pointIndex;
  }
  else
  {
    return 0;
  }
}

static inline VTKM_EXEC vtkm::IdComponent CellEdgeLocalIndex(vtkm::IdComponent numPoints,
                                                             vtkm::IdComponent pointIndex,
                                                             vtkm::IdComponent edgeIndex,
                                                             vtkm::CellShapeTagGeneric shape,
                                                             const vtkm::exec::FunctorBase& worklet)
{
  VTKM_ASSUME(pointIndex >= 0);
  VTKM_ASSUME(pointIndex < 2);
  VTKM_ASSUME(edgeIndex >= 0);
  VTKM_ASSUME(edgeIndex < detail::CellEdgeTables::MAX_NUM_EDGES);

  if (shape.Id == vtkm::CELL_SHAPE_POLYGON)
  {
    return CellEdgeLocalIndex(
      numPoints, pointIndex, edgeIndex, vtkm::CellShapeTagPolygon(), worklet);
  }
  else
  {
    detail::CellEdgeTables table;
    if (edgeIndex >= table.NumEdges(shape.Id))
    {
      worklet.RaiseError("Invalid edge number.");
      return 0;
    }

    return table.PointsInEdge(shape.Id, edgeIndex, pointIndex);
  }
}

/// \brief Returns a canonical identifier for a cell edge
///
/// Given information about a cell edge and the global point indices for that cell, returns a
/// vtkm::Id2 that contains values that are unique to that edge. The values for two edges will be
/// the same if and only if the edges contain the same points.
///
template <typename CellShapeTag, typename GlobalPointIndicesVecType>
static inline VTKM_EXEC vtkm::Id2 CellEdgeCanonicalId(
  vtkm::IdComponent numPoints,
  vtkm::IdComponent edgeIndex,
  CellShapeTag shape,
  const GlobalPointIndicesVecType& globalPointIndicesVec,
  const vtkm::exec::FunctorBase& worklet)
{
  vtkm::Id pointIndex0 =
    globalPointIndicesVec[vtkm::exec::CellEdgeLocalIndex(numPoints, 0, edgeIndex, shape, worklet)];
  vtkm::Id pointIndex1 =
    globalPointIndicesVec[vtkm::exec::CellEdgeLocalIndex(numPoints, 1, edgeIndex, shape, worklet)];
  if (pointIndex0 < pointIndex1)
  {
    return vtkm::Id2(pointIndex0, pointIndex1);
  }
  else
  {
    return vtkm::Id2(pointIndex1, pointIndex0);
  }
}
}
} // namespace vtkm::exec

#endif //vtk_m_exec_CellFaces_h
