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
  DataSetMesh.h
  MergeTree.h
  MeshExtrema.h
  NotNoSuchElementPredicate.h
  PrintVectors.h
  ProcessContourTree.h
  Types.h
  )
#-----------------------------------------------------------------------------
add_subdirectory(data_set_mesh)
add_subdirectory(meshtypes)
add_subdirectory(meshextrema)
add_subdirectory(activegraph)
add_subdirectory(contourtreemaker)
add_subdirectory(processcontourtree)

vtkm_declare_headers(${headers})
