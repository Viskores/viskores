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
#ifndef vtk_m_worklet_cellmetrics_CellShearMetric_h
#define vtk_m_worklet_cellmetrics_CellShearMetric_h

/*
 * Mesh quality metric function that computes the shear of a cell. The shear is
 * defined as the minimum of areas of the faces divided by the length of the
 * indexed face multiplied by the index - 1 mod num_faces.
 *
 * These metric computations are adapted from the VTK implementation of the
 * Verdict library, which provides a set of mesh/cell metrics for evaluating the
 * geometric qualities of regions of mesh spaces.
 *
 * See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
 * See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this
 * metric)
 */

#include "TypeOfCellHexahedral.h"
#include "TypeOfCellQuadrilateral.h"
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
VTKM_EXEC OutType CellShearMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  CellShapeType shape,
                                  const vtkm::exec::FunctorBase&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(-1.0);
}

// ========================= 2D cells ==================================

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellShearMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  vtkm::CellShapeTagQuad,
                                  const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("Diagonal ratio metric(quad) requires 4 points.");
    return OutType(0.0);
  }
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar alpha0 = GetQuadAlpha0<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar alpha1 = GetQuadAlpha1<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar alpha2 = GetQuadAlpha2<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar alpha3 = GetQuadAlpha3<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar L0 = GetQuadL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar L1 = GetQuadL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar L2 = GetQuadL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar L3 = GetQuadL3Magnitude<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar x0 = alpha0 / (L0 * L3);
  const Scalar x1 = alpha1 / (L1 * L0);
  const Scalar x2 = alpha2 / (L2 * L1);
  const Scalar x3 = alpha3 / (L3 * L2);

  const Scalar q = vtkm::Min(x0, vtkm::Min(x1, vtkm::Min(x2, x3)));

  return q;
}

// ========================= 3D cells ==================================

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellShearMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  vtkm::CellShapeTagHexahedron,
                                  const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 8)
  {
    worklet.RaiseError("Diagonal ratio metric(hex) requires 8 points.");
    return OutType(-1.0);
  }
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar a0 = GetHexAlphaiHat<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(0));
  const Scalar a1 = GetHexAlphaiHat<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(1));
  const Scalar a2 = GetHexAlphaiHat<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(2));
  const Scalar a3 = GetHexAlphaiHat<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(3));
  const Scalar a4 = GetHexAlphaiHat<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(4));
  const Scalar a5 = GetHexAlphaiHat<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(5));
  const Scalar a6 = GetHexAlphaiHat<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(6));
  const Scalar a7 = GetHexAlphaiHat<Scalar, Vector, CollectionOfPoints>(pts, vtkm::Id(7));

  const Scalar q = vtkm::Min(
    a0,
    vtkm::Min(a1, vtkm::Min(a2, vtkm::Min(a3, vtkm::Min(a4, vtkm::Min(a5, vtkm::Min(a6, a7)))))));

  return q;
}


} // namespace cellmetrics
} // namespace worklet
} // namespace vtkm

#endif // vtk_m_worklet_cellmetrics_CellEdgeRatioMetric_h
