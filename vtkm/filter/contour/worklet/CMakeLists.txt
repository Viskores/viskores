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
  ContourFlyingEdges.h
  ContourMarchingCells.h
  MIR.h
  )

add_subdirectory(clip)
add_subdirectory(contour)
add_subdirectory(mir)

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})
