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
#ifndef vtk_m_exec_cellmetrics_Jacobian_h
#define vtk_m_exec_cellmetrics_Jacobian_h

/*
 * Mesh quality metric functions that computes the jacobian of mesh cells.
 * The jacobian of a cell is defined as the determinant of the Jociabian matrix 
 *
 * These metric computations are adapted from the VTK implementation of the Verdict library,
 * which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions 
 * of mesh spaces. 
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
VTKM_EXEC inline OutType CellJacobianMetricOfQuad(const VecType& edgeCalculations,
                                                  const VecType& axes)
{
  const vtkm::Id numCalculations = edgeCalculations.GetNumberOfComponents();

  //Compare partitions of quad to find min
  using axesType = typename VecType::ComponentType;
  axesType centerCalculation = vtkm::Cross(axes[0], axes[1]);
  vtkm::Normalize(centerCalculation);

  OutType currCalculation, minCalculation = vtkm::Infinity<OutType>();

  for (vtkm::IdComponent i = 0; i < numCalculations; i++)
  {
    currCalculation = vtkm::Dot(edgeCalculations[i], centerCalculation);
    if (currCalculation < minCalculation)
      minCalculation = currCalculation;
  }
  if (minCalculation > 0)
    return vtkm::Min(minCalculation, vtkm::Infinity<OutType>()); //normal case

  return vtkm::Max(minCalculation, OutType(-1) * vtkm::Infinity<OutType>());
}

template <typename OutType, typename VecType>
VTKM_EXEC inline OutType CellJacobianMetricOfHex(const VecType& matrices)
{
  const vtkm::IdComponent numMatrices = matrices.GetNumberOfComponents();

  //Compare determinants to find min
  OutType currDeterminant, minDeterminant;
  //principle axes matrix computed outside of for loop to avoid un-necessary if statement
  minDeterminant =
    (OutType)vtkm::Dot(matrices[numMatrices - 1][0],
                       vtkm::Cross(matrices[numMatrices - 1][1], matrices[numMatrices - 1][2]));
  minDeterminant /= 64.0;
  for (vtkm::IdComponent i = 0; i < numMatrices - 1; i++)
  {
    currDeterminant =
      (OutType)vtkm::Dot(matrices[i][0], vtkm::Cross(matrices[i][1], matrices[i][2]));
    if (currDeterminant < minDeterminant)
      minDeterminant = currDeterminant;
  }

  if (minDeterminant > 0)
    return vtkm::Min(minDeterminant, vtkm::Infinity<OutType>()); //normal case

  return vtkm::Max(minDeterminant, vtkm::NegativeInfinity<OutType>());
}


// ========================= Unsupported cells ==================================

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VTKM_EXEC OutType CellJacobianMetric(const vtkm::IdComponent& numPts,
                                     const PointCoordVecType& pts,
                                     CellShapeType shape,
                                     const vtkm::exec::FunctorBase&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(0.0);
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellJacobianMetric(const vtkm::IdComponent& numPts,
                                     const PointCoordVecType& pts,
                                     vtkm::CellShapeTagPolygon,
                                     const vtkm::exec::FunctorBase& worklet)
{
  switch (numPts)
  {
    case 4:
      return CellJacobianMetric<OutType>(numPts, pts, vtkm::CellShapeTagQuad(), worklet);
    default:
      break;
  }
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellJacobianMetric(const vtkm::IdComponent&,
                                     const PointCoordVecType&,
                                     vtkm::CellShapeTagLine,
                                     const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(worklet);
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellJacobianMetric(const vtkm::IdComponent&,
                                     const PointCoordVecType&,
                                     vtkm::CellShapeTagTriangle,
                                     const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(worklet);
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellJacobianMetric(const vtkm::IdComponent&,
                                     const PointCoordVecType&,
                                     vtkm::CellShapeTagWedge,
                                     const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(worklet);
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellJacobianMetric(const vtkm::IdComponent&,
                                     const PointCoordVecType&,
                                     vtkm::CellShapeTagPyramid,
                                     const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(worklet);
  return OutType(-1.0);
}
// ========================= 2D cells ==================================
// Compute the jacobian of a quadrilateral.
// Formula: min{Jacobian at each vertex}
// Equals 1 for a unit square
// Acceptable range: [0,FLOAT_MAX]
// Normal range: [0,FLOAT_MAX]
// Full range: [FLOAT_MIN,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellJacobianMetric(const vtkm::IdComponent& numPts,
                                     const PointCoordVecType& pts,
                                     vtkm::CellShapeTagQuad,
                                     const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("Jacobian metric(quad) requires 4 points.");
    return OutType(0.0);
  }


  //The 4 edges of a quadrilateral
  using Edge = typename PointCoordVecType::ComponentType;
  const Edge QuadEdges[4] = { pts[1] - pts[0], pts[2] - pts[1], pts[3] - pts[2], pts[0] - pts[3] };
  const Edge QuadAxes[2] = { QuadEdges[0] - (pts[2] - pts[3]), QuadEdges[1] - (pts[3] - pts[0]) };
  const Edge QuadEdgesToUse[4] = { vtkm::Cross(QuadEdges[3], QuadEdges[0]),
                                   vtkm::Cross(QuadEdges[0], QuadEdges[1]),
                                   vtkm::Cross(QuadEdges[1], QuadEdges[2]),
                                   vtkm::Cross(QuadEdges[2], QuadEdges[3]) };
  return vtkm::exec::cellmetrics::CellJacobianMetricOfQuad<OutType>(
    vtkm::make_VecC(QuadEdgesToUse, 4), vtkm::make_VecC(QuadAxes, 2));
}

// ============================= 3D Volume cells ==================================
// Compute the jacobian of a hexahedron.
// Formula: min{ {Alpha_i}, Alpha_8*/64}
//	-Alpha_i -> jacobian determinant at respective vertex
//	-Alpha_8 -> jacobian at center
// Equals 1 for a unit cube
// Acceptable Range: [0, FLOAT_MAX]
// Normal Range: [0, FLOAT_MAX]
// Full range: [FLOAT_MIN ,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellJacobianMetric(const vtkm::IdComponent& numPts,
                                     const PointCoordVecType& pts,
                                     vtkm::CellShapeTagHexahedron,
                                     const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 8)
  {
    worklet.RaiseError("Jacobian metric(hexahedron) requires 8 points.");
    return OutType(0.0);
  }


  //The 12 edges of a hexahedron
  using Edge = typename PointCoordVecType::ComponentType;
  Edge HexEdges[12] = { pts[1] - pts[0], pts[2] - pts[1], pts[3] - pts[2], pts[3] - pts[0],
                        pts[4] - pts[0], pts[5] - pts[1], pts[6] - pts[2], pts[7] - pts[3],
                        pts[5] - pts[4], pts[6] - pts[5], pts[7] - pts[6], pts[7] - pts[4] };
  Edge principleXAxis = HexEdges[0] + (pts[2] - pts[3]) + HexEdges[8] + (pts[6] - pts[7]);
  Edge principleYAxis = HexEdges[3] + HexEdges[1] + HexEdges[11] + HexEdges[9];
  Edge principleZAxis = HexEdges[5] + HexEdges[6] + HexEdges[7] + HexEdges[8];

  const Edge hexMatrices[9][3] = { { HexEdges[0], HexEdges[3], HexEdges[4] },
                                   { HexEdges[1], -1 * HexEdges[0], HexEdges[5] },
                                   { HexEdges[2], -1 * HexEdges[1], HexEdges[6] },
                                   { -1 * HexEdges[3], -1 * HexEdges[2], HexEdges[7] },
                                   { HexEdges[11], HexEdges[8], -1 * HexEdges[4] },
                                   { -1 * HexEdges[8], HexEdges[9], -1 * HexEdges[5] },
                                   { -1 * HexEdges[9], HexEdges[10], -1 * HexEdges[6] },
                                   { -1 * HexEdges[10], -1 * HexEdges[11], -1 * HexEdges[7] },
                                   { principleXAxis, principleYAxis, principleZAxis } };
  return vtkm::exec::cellmetrics::CellJacobianMetricOfHex<OutType>(
    vtkm::make_VecC(hexMatrices, 12));
}

// Compute the jacobian of a tetrahedron.
// Formula: (L2 * L0) * L3
// Equals Sqrt(2) / 2 for unit equilateral tetrahedron
// Acceptable Range: [0, FLOAT_MAX]
// Normal Range: [0, FLOAT_MAX]
// Full range: [FLOAT_MIN,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellJacobianMetric(const vtkm::IdComponent& numPts,
                                     const PointCoordVecType& pts,
                                     vtkm::CellShapeTagTetra,
                                     const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("Jacobian metric requires 4 points");
    return OutType(0.0);
  }

  //the 3 edges we need
  using Edge = typename PointCoordVecType::ComponentType;
  const Edge EdgesNeeded[3] = { pts[1] - pts[0], pts[0] - pts[2], pts[3] - pts[0] };
  return (OutType)vtkm::Dot(vtkm::Cross(EdgesNeeded[1], EdgesNeeded[0]), EdgesNeeded[2]);
}

} // namespace cellmetrics
} // namespace exec
} // namespace vtkm

#endif // vtk_m_exec_cellmetrics_CellEdgeRatioMetric_h
