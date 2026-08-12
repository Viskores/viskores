##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
viskores_declare_headers(${headers})
