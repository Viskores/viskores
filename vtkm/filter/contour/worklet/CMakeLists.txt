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
  contour/CommonState.h
  Contour.h
  contour/FieldPropagation.h
  contour/FlyingEdges.h
  contour/FlyingEdgesHelpers.h
  contour/FlyingEdgesPass1.h
  contour/FlyingEdgesPass2.h
  contour/FlyingEdgesPass4.h
  contour/FlyingEdgesPass4Common.h
  contour/FlyingEdgesPass4X.h
  contour/FlyingEdgesPass4XWithNormals.h
  contour/FlyingEdgesPass4Y.h
  contour/FlyingEdgesTables.h
  contour/MarchingCellTables.h
  contour/MarchingCells.h
  )

add_subdirectory(clip)
add_subdirectory(contour)

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})
