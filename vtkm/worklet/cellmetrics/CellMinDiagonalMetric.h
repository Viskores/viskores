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
#ifndef vtk_m_worklet_cellmetrics_Min_Diagonal_h
#define vtk_m_worklet_cellmetrics_Min_Diagonal_h

/*
* Mesh quality metric functions that compute the Oddy of mesh cells.
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
VTKM_EXEC OutType CellMinDiagonalMetric(const vtkm::IdComponent& numPts,
                                        const PointCoordVecType& pts,
                                        CellShapeType shape,
                                        const vtkm::exec::FunctorBase&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(-1.0);
}
// ============================= 3D Volume cells ==================================
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellMinDiagonalMetric(const vtkm::IdComponent& numPts,
                                        const PointCoordVecType& pts,
                                        vtkm::CellShapeTagHexahedron,
                                        const vtkm::exec::FunctorBase& worklet)
{
  if (numPts != 8)
  {
    worklet.RaiseError("Min diagonal metric(hexahedron) requires 8 points.");
    return OutType(0.0);
  }

  using Scalar = OutType;

  Scalar temp[3], diag[4];
  vtkm::IdComponent i(0);

  //lengths^2  f diag nals
  for (i = 0; i < 3; i++)
  {
    temp[i] = pts[6][i] - pts[0][i];
    temp[i] = temp[i] * temp[i];
  }
  diag[0] = vtkm::Sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = pts[4][i] - pts[2][i];
    temp[i] = temp[i] * temp[i];
  }
  diag[1] = vtkm::Sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = pts[7][i] - pts[1][i];
    temp[i] = temp[i] * temp[i];
  }
  diag[2] = vtkm::Sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = pts[5][i] - pts[3][i];
    temp[i] = temp[i] * temp[i];
  }
  diag[3] = vtkm::Sqrt(temp[0] + temp[1] + temp[2]);

  Scalar diagonal = diag[0];

  for (i = 1; i < 4; i++)
  {
    diagonal = vtkm::Min(diagonal, diag[i]);
  }
  return Scalar(diagonal);
}
} // namespace cellmetrics
} // namespace worklet
} // namespace vtkm
#endif
