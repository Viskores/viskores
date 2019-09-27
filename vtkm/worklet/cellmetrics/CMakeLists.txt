##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  CellAspectFrobeniusMetric.h
  CellAspectGammaMetric.h
  CellAspectRatioMetric.h
  CellConditionMetric.h
  CellDiagonalRatioMetric.h
  CellDimensionMetric.h
  CellEdgeRatioMetric.h
  CellJacobianMetric.h
  CellMaxAngleMetric.h
  CellMaxAspectFrobeniusMetric.h
  CellMaxDiagonalMetric.h
  CellMinAngleMetric.h
  CellMinDiagonalMetric.h
  CellOddyMetric.h
  CellRelativeSizeSquaredMetric.h
  CellScaledJacobianMetric.h
  CellShapeMetric.h
  CellShapeAndSizeMetric.h
  CellShearMetric.h
  CellSkewMetric.h
  CellStretchMetric.h
  CellTaperMetric.h
  CellWarpageMetric.h
  TypeOfCellHexahedral.h
  TypeOfCellQuadrilateral.h
  TypeOfCellTetrahedral.h
  TypeOfCellTriangle.h
  )


#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})
