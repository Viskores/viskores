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
#ifndef vtk_m_worklet_cellmetrics_ScaledJacobian_h
#define vtk_m_worklet_cellmetrics_ScaledJacobian_h

/*
* Mesh quality metric functions that computes the scaled jacobian of mesh cells.
* The jacobian of a cell is defined as the determinant of the Jociabian matrix
** These metric computations are adapted from the VTK implementation of the Verdict library,
* which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
* of mesh spaces.
** See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
* See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this metric)
*/

#include "TypeOfCellHexahedral.h"
#include "TypeOfCellQuadrilateral.h"
#include "TypeOfCellTetrahedral.h"
#include "TypeOfCellTriangle.h"
#include "vtkm/CellShape.h"
#include "vtkm/CellTraits.h"
#include "vtkm/VecTraits.h"
#include "vtkm/VectorAnalysis.h"
#include "vtkm/exec/FunctorBase.h"

#define UNUSED(expr) (void)(expr);

namespace vtkm
{
namespace worklet
{
namespace cellmetrics
{

// ========================= Unsupported cells ==================================

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VTKM_EXEC OutType CellScaledJacobianMetric(const vtkm::IdComponent& numPts,
                                           const PointCoordVecType& pts,
                                           CellShapeType shape,
                                           const vtkm::exec::FunctorBase&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(-2.0);
}

// ========================= 2D cells ==================================
//Compute the scaled jacobian of a triangle
//Formula: q = ((2*sqrt(3))/3) * (J/Lmax)
//	- J -> jacobian, if surface normal N is center of triangle and
//	   and N*L2*L1 < 0, then -jacobian
//	- Lmax -> max{ |L0| * |L1|, |L0| * |L2|, |L1| * |L2| }
//Equals 1 for equilateral unit triangle
//Acceptable Range: [0.5, 2*sqrt(3)/3]
//Normal Range  : [ -(2*sqrt(3)/3) , 2*sqrt(3)/3]
//Full Range  : [-FLOAT_MAX, FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellScaledJacobianMetric(const vtkm::IdComponent& numPts,
                                           const PointCoordVecType& pts,
                                           vtkm::CellShapeTagTriangle,
                                           const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 3)
  {
    worklet.RaiseError("ScaledJacobian metric(tri) requires 3 points.");
    return OutType(0.0);
  }
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Vector l0 = GetTriangleL0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector l1 = GetTriangleL1<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector l2 = GetTriangleL2<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar modifier = (Scalar)(2 * vtkm::Sqrt(3) / 3);
  const Scalar l0_magnitude = GetTriangleL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l1_magnitude = GetTriangleL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l2_magnitude = GetTriangleL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l0l1_product = l0_magnitude * l1_magnitude;
  const Scalar l0l2_product = l0_magnitude * l2_magnitude;
  const Scalar l1l2_product = l1_magnitude * l2_magnitude;
  const Scalar productMax = vtkm::Max(l0l1_product, vtkm::Max(l0l2_product, l1l2_product));
  if (productMax <= Scalar(0.0))
  {
    return Scalar(0.0);
  }
  // compute jacobian of triangle
  Vector TriCross = vtkm::Cross(l2, l1);
  Scalar scaledJacobian = vtkm::Magnitude(TriCross);

  //add all pieces together
  //TODO change
  Vector normalVector = (Scalar(1.0) / Scalar(3.0)) * (l0 + l1 + l2);
  Vector surfaceNormalAtCenter = vtkm::TriangleNormal(normalVector, normalVector, normalVector);
  if (vtkm::Dot(surfaceNormalAtCenter, TriCross) < 0)
  {
    scaledJacobian *= -1;
  }
  scaledJacobian *= modifier;
  const Scalar q = scaledJacobian / productMax;

  return q;
}

// Compute the scaled jacobian of a quadrilateral.
// Formula: min{J0/(L0*L3), J1/(L1*L0), J2/(L2*L1), J3/(L3*L2)}
//	-Ji -> Jacobian at corner i, the intersection of the edge vectors
//	   it is divided by
// Equals 1 for a unit square
//Acceptable Range: [0.3, 1]
//Normal Range  : [-1, 1]
// Full range   : [-1, 1]
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellScaledJacobianMetric(const vtkm::IdComponent& numPts,
                                           const PointCoordVecType& pts,
                                           vtkm::CellShapeTagQuad,
                                           const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("ScaledJacobian metric(quad) requires 4 points.");
    return OutType(0.0);
  }
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;
  //The 4 edges of a quadrilateral
  const Scalar l0_magnitude = GetQuadL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l1_magnitude = GetQuadL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l2_magnitude = GetQuadL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l3_magnitude = GetQuadL3Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar negativeInfinity = vtkm::NegativeInfinity<Scalar>();

  if (l0_magnitude < negativeInfinity || l1_magnitude < negativeInfinity ||
      l2_magnitude < negativeInfinity || l3_magnitude < negativeInfinity)
  {
    return Scalar(0.0);
  }
  const Scalar l0l3_product = l0_magnitude * l3_magnitude;
  const Scalar l1l0_product = l1_magnitude * l0_magnitude;
  const Scalar l2l1_product = l2_magnitude * l1_magnitude;
  const Scalar l3l2_product = l3_magnitude * l2_magnitude;

  /*
3 * 0
0 * 1
1 * 2
2 * 3
*/

  Scalar alpha0Scaled = GetQuadAlpha0<Scalar, Vector, CollectionOfPoints>(pts);
  Scalar alpha1Scaled = GetQuadAlpha1<Scalar, Vector, CollectionOfPoints>(pts);
  Scalar alpha2Scaled = GetQuadAlpha2<Scalar, Vector, CollectionOfPoints>(pts);
  Scalar alpha3Scaled = GetQuadAlpha3<Scalar, Vector, CollectionOfPoints>(pts);

  alpha0Scaled /= l0l3_product;
  alpha1Scaled /= l1l0_product;
  alpha2Scaled /= l2l1_product;
  alpha3Scaled /= l3l2_product;
  const Scalar q =
    vtkm::Min(alpha0Scaled, vtkm::Min(alpha1Scaled, vtkm::Min(alpha2Scaled, alpha3Scaled)));
  return q;
}

// ============================= 3D Volume cells ==================================
// Compute the scaled jacobian of a hexahedron.
// Formula: q = min{Ai}
//	Ai -> for i 1...8 (Jacobian determinant at respective corner, divided by corresponding edge lengths
// Equals 1 for a unit cube
// Acceptable Range: [0.5, 1]
// Normal Range  : [-1, 1]
// Full range  : [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellScaledJacobianMetric(const vtkm::IdComponent& numPts,
                                           const PointCoordVecType& pts,
                                           vtkm::CellShapeTagHexahedron,
                                           const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 8)
  {
    worklet.RaiseError("ScaledJacobian metric(hexahedron) requires 8 points.");
    return OutType(0.0);
  }

  //The 12 edges of a hexahedron
  using Edge = typename PointCoordVecType::ComponentType;
  Edge HexEdges[12] = { pts[1] - pts[0], pts[2] - pts[1], pts[3] - pts[2], pts[3] - pts[0],
                        pts[4] - pts[0], pts[5] - pts[1], pts[6] - pts[2], pts[7] - pts[3],
                        pts[5] - pts[4], pts[6] - pts[5], pts[7] - pts[6], pts[7] - pts[4] };
  Edge principleXAxis = HexEdges[0] + (pts[2] - pts[3]) + HexEdges[8] + (pts[6] - pts[7]);
  Edge principleYAxis = (pts[3] - pts[0]) + HexEdges[1] + (pts[7] - pts[4]) + HexEdges[9];
  Edge principleZAxis = HexEdges[4] + HexEdges[5] + HexEdges[6] + HexEdges[7];
  Edge hexMatrices[9][3] = { { HexEdges[0], HexEdges[3], HexEdges[4] },
                             { HexEdges[1], (-1 * HexEdges[0]), HexEdges[5] },
                             { HexEdges[2], (-1 * HexEdges[1]), HexEdges[6] },
                             { (-1 * HexEdges[3]), (-1 * HexEdges[2]), HexEdges[7] },
                             { HexEdges[11], HexEdges[8], (-1 * HexEdges[4]) },
                             { (-1 * HexEdges[8]), HexEdges[9], (-1 * HexEdges[5]) },
                             { (-1 * HexEdges[9]), HexEdges[10], (-1 * HexEdges[6]) },
                             { (-1 * HexEdges[10]), (-1 * HexEdges[11]), (-1 * HexEdges[7]) },
                             { principleXAxis, principleYAxis, principleZAxis } };
  OutType currDeterminant, minDeterminant = vtkm::Infinity<OutType>();
  FloatType lenSquared1, lenSquared2, lenSquared3, minLengthSquared = vtkm::Infinity<FloatType>();
  vtkm::IdComponent matrixIndex;
  for (matrixIndex = 0; matrixIndex < 9; matrixIndex++)
  {
    lenSquared1 = (FloatType)vtkm::MagnitudeSquared(hexMatrices[matrixIndex][0]);
    minLengthSquared = lenSquared1 < minLengthSquared ? lenSquared1 : minLengthSquared;
    lenSquared2 = (FloatType)vtkm::MagnitudeSquared(hexMatrices[matrixIndex][1]);
    minLengthSquared = lenSquared2 < minLengthSquared ? lenSquared2 : minLengthSquared;
    lenSquared3 = (FloatType)vtkm::MagnitudeSquared(hexMatrices[matrixIndex][2]);
    minLengthSquared = lenSquared3 < minLengthSquared ? lenSquared3 : minLengthSquared;

    vtkm::Normalize(hexMatrices[matrixIndex][0]);
    vtkm::Normalize(hexMatrices[matrixIndex][1]);
    vtkm::Normalize(hexMatrices[matrixIndex][2]);
    currDeterminant =
      (OutType)vtkm::Dot(hexMatrices[matrixIndex][0],
                         vtkm::Cross(hexMatrices[matrixIndex][1], hexMatrices[matrixIndex][2]));
    if (currDeterminant < minDeterminant)
    {
      minDeterminant = currDeterminant;
    }
  }
  if (minLengthSquared < vtkm::NegativeInfinity<FloatType>())
  {
    return vtkm::Infinity<OutType>();
  }
  OutType toReturn = minDeterminant;
  if (toReturn > 0)
    return vtkm::Min(toReturn, vtkm::Infinity<OutType>()); //normal case

  return vtkm::Max(toReturn, vtkm::NegativeInfinity<OutType>());
}

// Compute the scaled jacobian of a tetrahedron.
// Formula: q = J*sqrt(2)/Lamda_max
//	J -> jacobian,(L2 * L0) * L3
//	Lamda_max -> max{ L0*L2*L3, L0*L1*L4, L1*L2*L5, L3*L4*L5}
// Equals Sqrt(2) / 2 for unit equilateral tetrahedron
// Acceptable Range: [0, FLOAT_MAX]
// Normal Range: [0, FLOAT_MAX]
// Full range: [FLOAT_MIN,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellScaledJacobianMetric(const vtkm::IdComponent& numPts,
                                           const PointCoordVecType& pts,
                                           vtkm::CellShapeTagTetra,
                                           const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("ScaledJacobian metric requires 4 points");
    return OutType(0.0);
  }

  //the edge and side sets
  using Edge = typename PointCoordVecType::ComponentType;
  Edge Edges[6] = { pts[1] - pts[0], pts[2] - pts[1], pts[0] - pts[2],
                    pts[3] - pts[0], pts[3] - pts[1], pts[3] - pts[2] };
  OutType EdgesSquared[6];
  OutType jacobian = vtkm::Dot(vtkm::Cross(Edges[2], Edges[0]), Edges[3]);
  // compute the scaled jacobian
  OutType currSide, maxSide = vtkm::NegativeInfinity<OutType>();
  vtkm::IdComponent edgeIndex, sideIndex;
  for (edgeIndex = 0; edgeIndex < 6; edgeIndex++)
  {
    EdgesSquared[edgeIndex] = vtkm::MagnitudeSquared(Edges[edgeIndex]);
  }
  OutType Sides[4] = { EdgesSquared[0] * EdgesSquared[2] * EdgesSquared[3],
                       EdgesSquared[0] * EdgesSquared[1] * EdgesSquared[4],
                       EdgesSquared[1] * EdgesSquared[2] * EdgesSquared[5],
                       EdgesSquared[3] * EdgesSquared[4] * EdgesSquared[5] };
  for (sideIndex = 0; sideIndex < 4; sideIndex++)
  {
    currSide = Sides[sideIndex];
    maxSide = currSide > maxSide ? currSide : maxSide;
  }
  maxSide = vtkm::Sqrt(maxSide);
  OutType toUseInCalculation = jacobian > maxSide ? jacobian : maxSide;
  if (toUseInCalculation < vtkm::NegativeInfinity<OutType>())
  {
    return vtkm::Infinity<OutType>();
  }
  return (vtkm::Sqrt<OutType>(2) * jacobian) / toUseInCalculation;
}
} // namespace cellmetrics
} // namespace worklet
} // namespace vtkm
#endif // vtk_m_worklet_cellmetrics_CellEdgeRatioMetric_h
