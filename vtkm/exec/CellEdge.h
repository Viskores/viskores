//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_CellFaces_h
#define vtk_m_exec_CellFaces_h

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

static const vtkm::IdComponent MAX_NUM_EDGES = 12;

VTKM_EXEC_CONSTANT
static const vtkm::IdComponent NumEdges[vtkm::NUMBER_OF_CELL_SHAPES] = {
  0,  //  0: CELL_SHAPE_EMPTY
  0,  //  1: CELL_SHAPE_VERTEX
  0,  //  2: Unused
  0,  //  3: CELL_SHAPE_LINE
  0,  //  4: Unused
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

VTKM_EXEC_CONSTANT
static const vtkm::IdComponent PointsInEdge[vtkm::NUMBER_OF_CELL_SHAPES][MAX_NUM_EDGES][2] = {
  //  0: CELL_SHAPE_EMPTY
  { { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  //  1: CELL_SHAPE_VERTEX
  { { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  //  2: Unused
  { { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  //  3: CELL_SHAPE_LINE
  { { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  //  4: Unused
  { { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  //  5: CELL_SHAPE_TRIANGLE
  { { 0, 1 },
    { 1, 2 },
    { 2, 0 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  //  6: Unused
  { { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  //  7: CELL_SHAPE_POLYGON  --- special case ---
  { { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  //  8: Unused
  { { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  //  9: CELL_SHAPE_QUAD
  { { 0, 1 },
    { 1, 2 },
    { 2, 3 },
    { 3, 0 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  // 10: CELL_SHAPE_TETRA
  { { 0, 1 },
    { 1, 2 },
    { 2, 0 },
    { 0, 3 },
    { 1, 3 },
    { 2, 3 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  // 11: Unused
  { { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  // 12: CELL_SHAPE_HEXAHEDRON
  { { 0, 1 },
    { 1, 2 },
    { 3, 2 },
    { 0, 3 },
    { 4, 5 },
    { 5, 6 },
    { 7, 6 },
    { 4, 7 },
    { 0, 4 },
    { 1, 5 },
    { 3, 7 },
    { 2, 6 } },
  // 13: CELL_SHAPE_WEDGE
  { { 0, 1 },
    { 1, 2 },
    { 2, 0 },
    { 3, 4 },
    { 4, 5 },
    { 5, 3 },
    { 0, 3 },
    { 1, 4 },
    { 2, 5 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
  // 14: CELL_SHAPE_PYRAMID
  { { 0, 1 },
    { 1, 2 },
    { 2, 3 },
    { 3, 0 },
    { 0, 4 },
    { 1, 4 },
    { 2, 4 },
    { 3, 4 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 } },
};

} // namespace detail

template <typename CellShapeTag>
static inline VTKM_EXEC vtkm::IdComponent CellEdgeNumberOfEdges(vtkm::IdComponent numPoints,
                                                                CellShapeTag,
                                                                const vtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  VTKM_ASSERT(numPoints == vtkm::CellTraits<CellShapeTag>::NUM_POINTS);
  return detail::NumEdges[CellShapeTag::Id];
}

static inline VTKM_EXEC vtkm::IdComponent CellEdgeNumberOfEdges(vtkm::IdComponent numPoints,
                                                                vtkm::CellShapeTagPolygon,
                                                                const vtkm::exec::FunctorBase&)
{
  VTKM_ASSUME(numPoints > 0);
  return numPoints;
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
  else
  {
    return detail::NumEdges[shape.Id];
  }
}

template <typename CellShapeTag>
static inline VTKM_EXEC vtkm::Vec<vtkm::IdComponent, 2> CellEdgeLocalIndices(
  vtkm::IdComponent numPoints,
  vtkm::IdComponent edgeIndex,
  CellShapeTag shape,
  const vtkm::exec::FunctorBase& worklet)
{
  VTKM_ASSUME(edgeIndex >= 0);
  VTKM_ASSUME(edgeIndex < detail::MAX_NUM_EDGES);
  if (edgeIndex >= vtkm::exec::CellEdgeNumberOfEdges(numPoints, shape, worklet))
  {
    worklet.RaiseError("Invalid edge number.");
    return vtkm::Vec<vtkm::IdComponent, 2>(0);
  }

  return vtkm::make_Vec(detail::PointsInEdge[CellShapeTag::Id][edgeIndex][0],
                        detail::PointsInEdge[CellShapeTag::Id][edgeIndex][1]);
}

static inline VTKM_EXEC vtkm::Vec<vtkm::IdComponent, 2> CellEdgeLocalIndices(
  vtkm::IdComponent numPoints,
  vtkm::IdComponent edgeIndex,
  vtkm::CellShapeTagPolygon,
  const vtkm::exec::FunctorBase&)
{
  VTKM_ASSUME(numPoints >= 3);
  VTKM_ASSUME(edgeIndex >= 0);
  VTKM_ASSUME(edgeIndex < numPoints);

  if (edgeIndex < numPoints - 1)
  {
    return vtkm::Vec<vtkm::IdComponent, 2>(edgeIndex, edgeIndex + 1);
  }
  else
  {
    return vtkm::Vec<vtkm::IdComponent, 2>(edgeIndex, 0);
  }
}

static inline VTKM_EXEC vtkm::Vec<vtkm::IdComponent, 2> CellEdgeLocalIndices(
  vtkm::IdComponent numPoints,
  vtkm::IdComponent edgeIndex,
  vtkm::CellShapeTagGeneric shape,
  const vtkm::exec::FunctorBase& worklet)
{
  VTKM_ASSUME(edgeIndex >= 0);
  VTKM_ASSUME(edgeIndex < detail::MAX_NUM_EDGES);

  if (shape.Id == vtkm::CELL_SHAPE_POLYGON)
  {
    return CellEdgeLocalIndices(numPoints, edgeIndex, vtkm::CellShapeTagPolygon(), worklet);
  }
  else
  {
    if (edgeIndex >= detail::NumEdges[shape.Id])
    {
      worklet.RaiseError("Invalid edge number.");
      return vtkm::Vec<vtkm::IdComponent, 2>(0);
    }

    return vtkm::make_Vec(detail::PointsInEdge[shape.Id][edgeIndex][0],
                          detail::PointsInEdge[shape.Id][edgeIndex][1]);
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
  vtkm::Vec<vtkm::IdComponent, 2> localPointIndices =
    vtkm::exec::CellEdgeLocalIndices(numPoints, edgeIndex, shape, worklet);

  vtkm::Id pointIndex0 = globalPointIndicesVec[localPointIndices[0]];
  vtkm::Id pointIndex1 = globalPointIndicesVec[localPointIndices[1]];
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
