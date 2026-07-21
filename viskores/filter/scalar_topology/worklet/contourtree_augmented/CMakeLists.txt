##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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

viskores_declare_headers(${headers})
