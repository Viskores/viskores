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
  TestingOpenGLInterop.h
  TestingTransferFancyHandles.h
  )
vtkm_declare_headers(${headers})


if(NOT TARGET GLUT::GLUT)
  find_package(GLUT QUIET)

  if(NOT TARGET GLUT::GLUT)
    return()
  endif()
endif()

# rendering can only have a single backend so we need to
if(VTKm_ENABLE_OSMESA_CONTEXT AND TARGET vtkm_rendering)
  list(APPEND unit_tests
    UnitTestTransferOSMesa.cxx
    UnitTestFancyTransferOSMesa.cxx
  )
elseif(VTKm_ENABLE_EGL_CONTEXT AND TARGET vtkm_rendering)
  list(APPEND unit_tests
    UnitTestTransferEGL.cxx
    UnitTestFancyTransferEGL.cxx
  )
elseif(VTKm_ENABLE_GL_CONTEXT AND TARGET vtkm_rendering)
  list(APPEND unit_tests
     UnitTestTransferGLUT.cxx
     UnitTestFancyTransferGLUT.cxx
  )
endif()

if(unit_tests)
  vtkm_unit_tests(SOURCES ${unit_tests}
                  LIBRARIES vtkm_rendering GLUT::GLUT
                  DEFINES VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)

endif()
