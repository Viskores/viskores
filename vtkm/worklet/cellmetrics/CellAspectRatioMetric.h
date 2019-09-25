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
#ifndef vtk_m_worklet_cellmetrics_CellAspectRatioMetric_h
#define vtk_m_worklet_cellmetrics_CellAspectRatioMetric_h

/*
* Mesh quality metric functions that compute the aspect ratio of mesh cells.
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
// The Verdict Manual and the Implementation have conflicting definitions.
// This duplicates the Verdict implementation in the VTKm Paradigm, with prior Manual
// definitions commented out when formerly coded.
// ========================= Unsupported cells ==================================

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VTKM_EXEC OutType CellAspectRatioMetric(const vtkm::IdComponent& numPts,
                                        const PointCoordVecType& pts,
                                        CellShapeType shape,
                                        const vtkm::exec::FunctorBase&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(0);
}

// ========================= 2D cells ==================================

// Compute the diagonal ratio of a triangle.
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellAspectRatioMetric(const vtkm::IdComponent& numPts,
                                        const PointCoordVecType& pts,
                                        vtkm::CellShapeTagTriangle,
                                        const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 3)
  {
    worklet.RaiseError("Aspect ratio metric (triangle) requires 3 points.");
    return OutType(0.0);
  }

  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar lmax = GetTriangleLMax<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar r = GetTriangleInradius<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar hhalf(0.5);
  const Scalar three(3.0);
  const Scalar q = (lmax * hhalf * vtkm::RSqrt(three)) / r;
  return q;
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellAspectRatioMetric(const vtkm::IdComponent& numPts,
                                        const PointCoordVecType& pts,
                                        vtkm::CellShapeTagQuad,
                                        const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("Aspect ratio metric (quad) requires 4 points.");
    return OutType(0.0);
  }

  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Vector X1 = GetQuadX0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector X2 = GetQuadX1<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar x1 = vtkm::Sqrt(vtkm::MagnitudeSquared(X1));
  const Scalar x2 = vtkm::Sqrt(vtkm::MagnitudeSquared(X2));
  if (x1 <= Scalar(0.0) || x2 <= Scalar(0.0))
  {
    return vtkm::Infinity<Scalar>();
  }

  const Scalar q = vtkm::Max(x1 / x2, x2 / x1);
  return q;
}

// ========================= 3D cells ==================================
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellAspectRatioMetric(const vtkm::IdComponent& numPts,
                                        const PointCoordVecType& pts,
                                        vtkm::CellShapeTagHexahedron,
                                        const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 8)
  {
    worklet.RaiseError("Aspect ratio metric (hex) requires 4 points.");
    return OutType(0.0);
  }

  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Vector X1 = GetHexX1<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector X2 = GetHexX2<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector X3 = GetHexX3<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar x1 = vtkm::Sqrt(vtkm::MagnitudeSquared(X1));
  const Scalar x2 = vtkm::Sqrt(vtkm::MagnitudeSquared(X2));
  const Scalar x3 = vtkm::Sqrt(vtkm::MagnitudeSquared(X3));

  if (x1 <= Scalar(0.0) || x2 <= Scalar(0.0) || x3 <= Scalar(0.0))
  {
    return vtkm::Infinity<Scalar>();
  }

  const Scalar q = vtkm::Max(
    x1 / x2,
    vtkm::Max(x2 / x1, vtkm::Max(x1 / x3, vtkm::Max(x3 / x1, vtkm::Max(x3 / x2, x3 / x2)))));
  return q;
}

// Compute the aspect ratio of a tetrahedron.
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellAspectRatioMetric(const vtkm::IdComponent& numPts,
                                        const PointCoordVecType& pts,
                                        vtkm::CellShapeTagTetra,
                                        const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("Aspect ratio metric (tetrahedron) requires 4 points.");
    return OutType(0.0);
  }

  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar rootSixInvert = vtkm::RSqrt(Scalar(6.0));
  const Scalar hhalf(0.5);
  const Scalar lmax = GetTetraLMax<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar r = GetTetraInradius<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar q = (hhalf * rootSixInvert * lmax) / r;
  return q;
}
} // namespace cellmetrics
} // namespace worklet
} // namespace vtkm
#endif // vtk_m_worklet_cellmetrics_CellAspectRatioMetric_h
