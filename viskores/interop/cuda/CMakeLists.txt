##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  SetOpenGLDevice.h
  )

#-----------------------------------------------------------------------------
viskores_declare_headers(${headers})

add_subdirectory(internal)

if(Viskores_ENABLE_OPENGL_TESTS AND Viskores_ENABLE_TESTING)
  add_subdirectory(testing)
endif()
