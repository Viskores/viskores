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
#ifndef vtk_m_worklet_CellShapeMetric_h
#define vtk_m_worklet_CellShapeMetric_h
/*
* Mesh quality metric functions that compute the shape, or weighted Jacobian, of mesh cells.
* The Jacobian of a cell is weighted by the condition metric value of the cell.
** These metric computations are adapted from the VTK implementation of the Verdict library,
* which provides a set of cell metrics for evaluating the geometric qualities of regions of mesh spaces.
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
#include "vtkm/worklet/cellmetrics/CellConditionMetric.h"
#include "vtkm/worklet/cellmetrics/CellJacobianMetric.h"

namespace vtkm
{
namespace worklet
{
namespace cellmetrics
{

// By default, cells have no shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VTKM_EXEC OutType CellShapeMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  CellShapeType shape,
                                  const vtkm::exec::FunctorBase&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(-1.0);
}

// =============================== Shape metric cells ==================================

// Compute the shape quality metric of a triangular cell.
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellShapeMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  vtkm::CellShapeTagTriangle,
                                  const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 3)
  {
    worklet.RaiseError("Shape metric(triangle) requires 3 points.");
    return OutType(0.0);
  }
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;

  const Scalar condition =
    vtkm::worklet::cellmetrics::CellConditionMetric<Scalar, CollectionOfPoints>(
      numPts, pts, vtkm::CellShapeTagTriangle(), worklet);
  const Scalar q(1 / condition);

  return q;
}

/// Compute the shape of a quadrilateral.
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellShapeMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  vtkm::CellShapeTagQuad,
                                  const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("Area(quad) requires 4 points.");
    return OutType(0.0);
  }

  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;
  const Scalar two(2.0);
  const Scalar alpha0 = GetQuadAlpha0<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar alpha1 = GetQuadAlpha1<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar alpha2 = GetQuadAlpha2<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar alpha3 = GetQuadAlpha3<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l0Squared =
    vtkm::Pow(GetQuadL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts), 2);
  const Scalar l1Squared =
    vtkm::Pow(GetQuadL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts), 2);
  const Scalar l2Squared =
    vtkm::Pow(GetQuadL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts), 2);
  const Scalar l3Squared =
    vtkm::Pow(GetQuadL3Magnitude<Scalar, Vector, CollectionOfPoints>(pts), 2);

  const Scalar min = vtkm::Min(
    (alpha0 / (l0Squared + l3Squared)),
    vtkm::Min((alpha1 / (l1Squared + l0Squared)),
              vtkm::Min((alpha2 / (l2Squared + l1Squared)), (alpha3 / (l3Squared + l2Squared)))));
  const Scalar q(two * min);

  return q;
}

// ============================= 3D cells ==================================
/// Compute the shape of a tetrahedron.
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellShapeMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  vtkm::CellShapeTagTetra,
                                  const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("Shape metric(tetrahedron) requires 4 points.");
    return OutType(0.0);
  }
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar zero(0.0);
  const Scalar twoThirds = (Scalar)(2.0 / 3.0);
  const Scalar threeHalves(1.5);
  const Scalar rtTwo = (Scalar)(vtkm::Sqrt(2.0));
  const Scalar three(3.0);
  const Scalar jacobian =
    vtkm::worklet::cellmetrics::CellJacobianMetric<Scalar, CollectionOfPoints>(
      numPts, pts, vtkm::CellShapeTagTetra(), worklet);

  if (jacobian <= zero)
  {
    return zero;
  }

  const Vector l0 = GetTetraL0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector l2 = GetTetraL2<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector l3 = GetTetraL3<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector negl2 = -1 * l2;

  const Scalar l0l0 = vtkm::Dot(l0, l0);
  const Scalar l2l2 = vtkm::Dot(l2, l2);
  const Scalar l3l3 = vtkm::Dot(l3, l3);
  const Scalar l0negl2 = vtkm::Dot(l0, negl2);
  const Scalar l0l3 = vtkm::Dot(l0, l3);
  const Scalar negl2l3 = vtkm::Dot(negl2, l3);

  const Scalar numerator = three * vtkm::Pow(jacobian * rtTwo, twoThirds);
  Scalar denominator = (threeHalves * (l0l0 + l2l2 + l3l3)) - (l0negl2 + l0l3 + negl2l3);
  if (denominator <= zero)
  {
    return zero;
  }
  Scalar q(numerator / denominator);
  return q;
}

/// Compute the shape of a hexahedral cell.
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellShapeMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  vtkm::CellShapeTagHexahedron,
                                  const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 8)
  {
    worklet.RaiseError("Volume(hexahedron) requires 8 points.");
    return OutType(0.0);
  }
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar zero(0.0);
  const Scalar twoThirds = (Scalar)(2.0 / 3.0);

  const Scalar alpha0 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(0));
  const Scalar alpha1 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(1));
  const Scalar alpha2 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(2));
  const Scalar alpha3 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(3));
  const Scalar alpha4 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(4));
  const Scalar alpha5 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(5));
  const Scalar alpha6 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(6));
  const Scalar alpha7 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(7));
  const Scalar alpha8 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(8));

  const Scalar A0squared =
    GetHexAiNormSquared<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(0));
  const Scalar A1squared =
    GetHexAiNormSquared<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(1));
  const Scalar A2squared =
    GetHexAiNormSquared<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(2));
  const Scalar A3squared =
    GetHexAiNormSquared<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(3));
  const Scalar A4squared =
    GetHexAiNormSquared<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(4));
  const Scalar A5squared =
    GetHexAiNormSquared<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(5));
  const Scalar A6squared =
    GetHexAiNormSquared<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(6));
  const Scalar A7squared =
    GetHexAiNormSquared<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(7));
  const Scalar A8squared =
    GetHexAiNormSquared<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(8));

  if (alpha0 <= zero || alpha1 <= zero || alpha2 <= zero || alpha3 <= zero || alpha4 <= zero ||
      alpha5 <= zero || alpha6 <= zero || alpha7 <= zero || alpha8 <= zero || A0squared <= zero ||
      A1squared <= zero || A2squared <= zero || A3squared <= zero || A4squared <= zero ||
      A5squared <= zero || A6squared <= zero || A7squared <= zero || A8squared <= zero)
  {
    return zero;
  }
  // find min according to verdict manual
  Scalar min = vtkm::Pow(alpha0, twoThirds) / A0squared;
  Scalar temp = vtkm::Pow(alpha1, twoThirds) / A1squared;
  min = temp < min ? temp : min;

  temp = vtkm::Pow(alpha2, twoThirds) / A2squared;
  min = temp < min ? temp : min;

  temp = vtkm::Pow(alpha3, twoThirds) / A3squared;
  min = temp < min ? temp : min;

  temp = vtkm::Pow(alpha4, twoThirds) / A4squared;
  min = temp < min ? temp : min;

  temp = vtkm::Pow(alpha5, twoThirds) / A5squared;
  min = temp < min ? temp : min;

  temp = vtkm::Pow(alpha6, twoThirds) / A6squared;
  min = temp < min ? temp : min;

  temp = vtkm::Pow(alpha7, twoThirds) / A7squared;
  min = temp < min ? temp : min;

  temp = vtkm::Pow(alpha8, twoThirds) / A8squared;
  min = temp < min ? temp : min;
  // determine the shape
  Scalar q(3 * min);
  return q;
}
} // cellmetrics
} // worklet
} // vtkm
#endif // vtk_m_worklet_CellShapeMetric_h
