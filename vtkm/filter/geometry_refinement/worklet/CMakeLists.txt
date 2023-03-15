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
  Shrink.h
  SplitSharpEdges.h
  Tetrahedralize.h
  Triangulate.h
  Tube.h
  VertexClustering.h
  )

add_subdirectory(tetrahedralize)
add_subdirectory(triangulate)

vtkm_declare_headers(${headers})
