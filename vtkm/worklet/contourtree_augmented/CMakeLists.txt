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
  ArrayTransforms.h
  ActiveGraph.h
  ContourTree.h
  ContourTreeMaker.h
  MergeTree.h
  Mesh_DEM_Triangulation.h
  MeshExtrema.h
  PointerDoubling.h
  PrintVectors.h
  ProcessContourTree.h
  Types.h
  EulerTour.h
  )
#-----------------------------------------------------------------------------
add_subdirectory(mesh_dem)
add_subdirectory(mesh_dem_meshtypes)
add_subdirectory(meshextrema)
add_subdirectory(activegraph)
add_subdirectory(contourtreemaker)
add_subdirectory(processcontourtree)

vtkm_declare_headers(${headers})
