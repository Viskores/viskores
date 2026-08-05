##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
viskores_declare_headers(${headers})
