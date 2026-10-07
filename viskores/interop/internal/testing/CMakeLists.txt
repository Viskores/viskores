##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


if(NOT TARGET GLEW::GLEW)
  return()
endif()

set(unit_tests
    UnitTestOpenGLHeaders.cxx
    UnitTestBufferTypePicker.cxx
  )

viskores_unit_tests(SOURCES ${unit_tests} LIBRARIES GLEW::GLEW)
