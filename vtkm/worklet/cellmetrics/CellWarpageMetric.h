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
#ifndef vtk_m_worklet_CellWarpageMetric_h
#define vtk_m_worklet_CellWarpageMetric_h

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

template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VTKM_EXEC OutType CellWarpageMetric(const vtkm::IdComponent& numPts,
                                    const PointCoordVecType& pts,
                                    CellShapeType shape,
                                    const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  UNUSED(worklet);
  //worklet.RaiseError("Shape type template must be Quad to compute warpage");
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellWarpageMetric(const vtkm::IdComponent& numPts,
                                    const PointCoordVecType& pts,
                                    vtkm::CellShapeTagQuad,
                                    const vtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(worklet);
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Vector N0Mag = GetQuadN0Normalized<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N1Mag = GetQuadN1Normalized<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N2Mag = GetQuadN2Normalized<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N3Mag = GetQuadN3Normalized<Scalar, Vector, CollectionOfPoints>(pts);

  if (N0Mag < Scalar(0.0) || N1Mag < Scalar(0.0) || N2Mag < Scalar(0.0) || N3Mag < Scalar(0.0))
    return vtkm::Infinity<Scalar>();
  const Scalar n0dotn2 = vtkm::Dot(N0Mag, N2Mag);
  const Scalar n1dotn3 = vtkm::Dot(N1Mag, N3Mag);
  const Scalar min = vtkm::Min(n0dotn2, n1dotn3);

  const Scalar minCubed = vtkm::Pow(min, 3);
  //return Scalar(1.0) - minCubed; // AS DEFINED IN THE MANUAL
  return minCubed; // AS DEFINED IN VISIT SOURCE CODE
}
}
} // worklet
} // vtkm
#endif // vtk_m_worklet_CellWarpage_Metric_h
