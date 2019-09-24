//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_cellmetrics_CellDiagonalRatioMetric_h
#define vtk_m_exec_cellmetrics_CellDiagonalRatioMetric_h

/*
 * Mesh quality metric functions that compute the diagonal ratio of mesh cells.
 * The diagonal ratio of a cell is defined as the length (magnitude) of the longest
 * cell diagonal length divided by the length of the shortest cell diagonal length.
 *
 * These metric computations are adapted from the VTK implementation of the Verdict library,
 * which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
 * of mesh spaces.
 *
 * The edge ratio computations for a pyramid cell types is not defined in the
 * VTK implementation, but is provided here.
 *
 * See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
 * See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this metric)
 */

#include "vtkm/CellShape.h"
#include "vtkm/CellTraits.h"
#include "vtkm/VecTraits.h"
#include "vtkm/VectorAnalysis.h"
#include "vtkm/exec/FunctorBase.h"

#define UNUSED(expr) (void)(expr);

namespace vtkm
{
namespace exec
{
namespace cellmetrics
{

using FloatType = vtkm::FloatDefault;


template <typename OutType, typename VecType>
VTKM_EXEC inline OutType ComputeDiagonalRatio(const VecType& diagonals)
{
  const vtkm::Id numDiagonals = diagonals.GetNumberOfComponents();

  //Compare diagonal lengths to determine the longest and shortest
  //TODO: Could we use lambda expression here?

  FloatType d0Len = (FloatType)vtkm::MagnitudeSquared(diagonals[0]);
  FloatType currLen, minLen = d0Len, maxLen = d0Len;
  for (int i = 1; i < numDiagonals; i++)
  {
    currLen = (FloatType)vtkm::MagnitudeSquared(diagonals[i]);
    if (currLen < minLen)
      minLen = currLen;
    if (currLen > maxLen)
      maxLen = currLen;
  }

  if (minLen < vtkm::NegativeInfinity<FloatType>())
    return vtkm::Infinity<OutType>();

  //Take square root because we only did magnitude squared before
  OutType diagonalRatio = (OutType)vtkm::Sqrt(maxLen / minLen);
  if (diagonalRatio > 0)
    return vtkm::Min(diagonalRatio, vtkm::Infinity<OutType>()); //normal case

  return vtkm::Max(diagonalRatio, OutType(-1) * vtkm::Infinity<OutType>());
}


// ========================= Unsupported cells ==================================

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VTKM_EXEC OutType CellDiagonalRatioMetric(const vtkm::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          CellShapeType shape,
                                          const vtkm::exec::FunctorBase&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(0.0);
}

/*
//TODO: Should polygons be supported? Maybe call Quad or Triangle function...
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellDiagonalRatioMetric(const vtkm::IdComponent& numPts,
                                 const PointCoordVecType& pts,
                                 vtkm::CellShapeTagPolygon,
                                 const vtkm::exec::FunctorBase& worklet)
{
  switch (numPts)
  {
    case 4:
            return CellDiagonalRatioMetric<OutType>(numPts, pts, vtkm::CellShapeTagQuad(), worklet);
    default:
            break;
  }
  return OutType(-1.0);
}
*/

/*
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellDiagonalRatioMetric(const vtkm::IdComponent&,
                                 const PointCoordVecType&,
                                 vtkm::CellShapeTagLine,
                                 const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(worklet);
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellDiagonalRatioMetric(const vtkm::IdComponent&,
                                 const PointCoordVecType&,
                                 vtkm::CellShapeTagTriangle,
                                 const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(worklet);
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellDiagonalRatioMetric(const vtkm::IdComponent&,
                                 const PointCoordVecType&,
                                 vtkm::CellShapeTagTetra,
                                 const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(worklet);
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellDiagonalRatioMetric(const vtkm::IdComponent&,
                                 const PointCoordVecType&,
                                 vtkm::CellShapeTagWedge,
                                 const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(worklet);
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellDiagonalRatioMetric(const vtkm::IdComponent&,
                                 const PointCoordVecType&,
                                 vtkm::CellShapeTagPyramid,
                                 const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(worklet);
  return OutType(-1.0);
}
*/

// ========================= 2D cells ==================================
// Compute the diagonal ratio of a quadrilateral.
// Formula: Maximum diagonal length divided by minimum diagonal length
// Equals 1 for a unit square
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellDiagonalRatioMetric(const vtkm::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          vtkm::CellShapeTagQuad,
                                          const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("Diagonal ratio metric(quad) requires 4 points.");
    return OutType(0.0);
  }

  vtkm::IdComponent numDiagonals = 2; //pts.GetNumberOfComponents();

  //The 2 diagonals of a quadrilateral
  using Diagonal = typename PointCoordVecType::ComponentType;
  const Diagonal QuadDiagonals[2] = { pts[2] - pts[0], pts[3] - pts[1] };

  return vtkm::exec::cellmetrics::ComputeDiagonalRatio<OutType>(
    vtkm::make_VecC(QuadDiagonals, numDiagonals));
}

// ============================= 3D Volume cells ==================================
// Compute the diagonal ratio of a hexahedron.
// Formula: Maximum diagonal length divided by minimum diagonal length
// Equals 1 for a unit cube
// Acceptable Range: [0.65, 1]
// Normal Range: [0, 1]
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellDiagonalRatioMetric(const vtkm::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          vtkm::CellShapeTagHexahedron,
                                          const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 8)
  {
    worklet.RaiseError("Diagonal ratio metric(hexahedron) requires 8 points.");
    return OutType(0.0);
  }

  vtkm::IdComponent numDiagonals = 4; //pts.GetNumberOfComponents();

  //The 4 diagonals of a hexahedron
  using Diagonal = typename PointCoordVecType::ComponentType;
  const Diagonal HexDiagonals[4] = {
    pts[6] - pts[0], pts[7] - pts[1], pts[4] - pts[2], pts[5] - pts[3]
  };

  return vtkm::exec::cellmetrics::ComputeDiagonalRatio<OutType>(
    vtkm::make_VecC(HexDiagonals, numDiagonals));
}

} // namespace cellmetrics
} // namespace exec
} // namespace vtkm

#endif // vtk_m_exec_cellmetrics_CellEdgeRatioMetric_h
