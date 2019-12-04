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
#ifndef vtk_m_worklet_CellSkewMetric_h
#define vtk_m_worklet_CellSkewMetric_h
/*
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

namespace vtkm
{
namespace worklet
{
namespace cellmetrics
{
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VTKM_EXEC OutType CellSkewMetric(const vtkm::IdComponent& numPts,
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

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellSkewMetric(const vtkm::IdComponent& numPts,
                                 const PointCoordVecType& pts,
                                 vtkm::CellShapeTagHexahedron,
                                 const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(worklet);
  using Scalar = OutType;
  using Vector = typename PointCoordVecType::ComponentType;
  Vector X1 = (pts[1] - pts[0]) + (pts[2] - pts[3]) + (pts[5] - pts[4]) + (pts[6] - pts[7]);
  Scalar X1_mag = vtkm::Magnitude(X1);
  if (X1_mag <= Scalar(0.0))
    return vtkm::Infinity<Scalar>();
  Vector x1 = X1 / X1_mag;
  Vector X2 = (pts[3] - pts[0]) + (pts[2] - pts[1]) + (pts[7] - pts[4]) + (pts[6] - pts[5]);
  Scalar X2_mag = vtkm::Magnitude(X2);
  if (X2_mag <= Scalar(0.0))
    return vtkm::Infinity<Scalar>();
  Vector x2 = X2 / X2_mag;
  Vector X3 = (pts[4] - pts[0]) + (pts[5] - pts[1]) + (pts[6] - pts[2]) + (pts[7] - pts[3]);
  Scalar X3_mag = vtkm::Magnitude(X3);
  if (X3_mag <= Scalar(0.0))
    return vtkm::Infinity<Scalar>();
  Vector x3 = X3 / X3_mag;
  return vtkm::Max(vtkm::Dot(x1, x2), vtkm::Max(vtkm::Dot(x1, x3), vtkm::Dot(x2, x3)));
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellSkewMetric(const vtkm::IdComponent& numPts,
                                 const PointCoordVecType& pts,
                                 vtkm::CellShapeTagQuad,
                                 const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(worklet);
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;
  const Vector X0 = GetQuadX0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector X1 = GetQuadX1<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar X0Mag = vtkm::Magnitude(X0);
  const Scalar X1Mag = vtkm::Magnitude(X1);

  if (X0Mag < Scalar(0.0) || X1Mag < Scalar(0.0))
    return Scalar(0.0);
  const Vector x0Normalized = X0 / X0Mag;
  const Vector x1Normalized = X1 / X1Mag;
  const Scalar dot = vtkm::Dot(x0Normalized, x1Normalized);
  return vtkm::Abs(dot);
}
}
} // worklet
} // vtkm
#endif
