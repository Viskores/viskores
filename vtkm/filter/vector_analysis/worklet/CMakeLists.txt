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
  Gradient.h
  Magnitude.h
  OrientCellNormals.h
  OrientNormals.h
  OrientPointAndCellNormals.h
  OrientPointNormals.h
  SurfaceNormals.h
  )

add_subdirectory(gradient)

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})
