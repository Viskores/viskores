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
#ifndef vtk_m_worklet_CellTaperMetric_h
#define vtk_m_worklet_CellTaperMetric_h
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

namespace vtkm
{
namespace worklet
{
namespace cellmetrics
{
// ========================= Unsupported cells ==================================

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VTKM_EXEC OutType CellTaperMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  CellShapeType shape,
                                  const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  UNUSED(worklet);
  return OutType(-1.0);
}
// ========================= 2D cells ==================================
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellTaperMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  vtkm::CellShapeTagQuad,
                                  const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(worklet);
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Vector X12 = Vector((pts[0] - pts[1]) + (pts[2] - pts[3]));
  const Vector X1 = GetQuadX0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector X2 = GetQuadX1<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar x12 = vtkm::Sqrt(vtkm::MagnitudeSquared(X12));
  const Scalar x1 = vtkm::Sqrt(vtkm::MagnitudeSquared(X1));
  const Scalar x2 = vtkm::Sqrt(vtkm::MagnitudeSquared(X2));
  const Scalar minLength = vtkm::Min(x1, x2);

  if (minLength <= Scalar(0.0))
  {
    return vtkm::Infinity<Scalar>();
  }

  const Scalar q = x12 / minLength;
  return q;
}

// ========================= 3D cells ==================================

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellTaperMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  vtkm::CellShapeTagHexahedron,
                                  const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(worklet);
  using Scalar = OutType;

  Scalar X1 = vtkm::Sqrt(vtkm::MagnitudeSquared((pts[1] - pts[0]) + (pts[2] - pts[3]) +
                                                (pts[5] - pts[4]) + (pts[6] - pts[7])));
  Scalar X2 = vtkm::Sqrt(vtkm::MagnitudeSquared((pts[3] - pts[0]) + (pts[2] - pts[1]) +
                                                (pts[7] - pts[4]) + (pts[6] - pts[5])));
  Scalar X3 = vtkm::Sqrt(vtkm::MagnitudeSquared((pts[4] - pts[0]) + (pts[5] - pts[1]) +
                                                (pts[6] - pts[2]) + (pts[7] - pts[3])));
  if ((X1 <= Scalar(0.0)) || (X2 <= Scalar(0.0)) || (X3 <= Scalar(0.0)))
  {
    return vtkm::Infinity<Scalar>();
  }
  Scalar X12 = vtkm::Sqrt(vtkm::MagnitudeSquared(((pts[2] - pts[3]) - (pts[1] - pts[0])) +
                                                 ((pts[6] - pts[7]) - (pts[5] - pts[4]))));
  Scalar X13 = vtkm::Sqrt(vtkm::MagnitudeSquared(((pts[5] - pts[1]) - (pts[4] - pts[0])) +
                                                 ((pts[6] - pts[2]) - (pts[7] - pts[3]))));
  Scalar X23 = vtkm::Sqrt(vtkm::MagnitudeSquared(((pts[7] - pts[4]) - (pts[3] - pts[0])) +
                                                 ((pts[6] - pts[5]) - (pts[2] - pts[1]))));
  Scalar T12 = X12 / vtkm::Min(X1, X2);
  Scalar T13 = X13 / vtkm::Min(X1, X3);
  Scalar T23 = X23 / vtkm::Min(X2, X3);
  return vtkm::Max(T12, vtkm::Max(T13, T23));
}
}
} // worklet
} // vtkm
#endif // vtk_m_worklet_CellTaper_Metric_h
