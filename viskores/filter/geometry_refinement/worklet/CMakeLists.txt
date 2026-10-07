##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  Extrusion.h
  ExtrusionLinear.h
  ExtrusionRotational.h
  Shrink.h
  SplitSharpEdges.h
  Tetrahedralize.h
  Triangulate.h
  Tube.h
  VertexClustering.h
  )

add_subdirectory(tetrahedralize)
add_subdirectory(triangulate)

viskores_declare_headers(${headers})
