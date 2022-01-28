##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  Clip.h
  CommonState.h
  Contour.h
  FieldPropagation.h
  FlyingEdges.h
  FlyingEdgesHelpers.h
  FlyingEdgesPass1.h
  FlyingEdgesPass2.h
  FlyingEdgesPass4.h
  FlyingEdgesPass4Common.h
  FlyingEdgesPass4X.h
  FlyingEdgesPass4XWithNormals.h
  FlyingEdgesPass4Y.h
  FlyingEdgesTables.h
  MarchingCellTables.h
  MarchingCells.h
  )

add_subdirectory(clip)

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})
