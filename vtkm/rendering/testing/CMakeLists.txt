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
  RenderTest.h
  )

vtkm_declare_headers(${headers})

set(unit_tests
  UnitTestCanvas.cxx
  UnitTestMapperConnectivity.cxx
  UnitTestMultiMapper.cxx
  #UnitTestMapperCylinders.cxx
  UnitTestMapperPoints.cxx
  UnitTestMapperQuads.cxx
  UnitTestMapperRayTracer.cxx
  UnitTestMapperWireframer.cxx
  UnitTestMapperVolume.cxx
)

vtkm_unit_tests(SOURCES ${unit_tests} ALL_BACKENDS LIBRARIES vtkm_rendering)

if(VTKm_ENABLE_GL_CONTEXT)
  # message(STATUS "rendering testing/glfw needs a FindGLFW")
  # message(STATUS "rendering testing/glut needs compile corrections")
  # add_subdirectory(glfw)
  # add_subdirectory(glut)
elseif(VTKm_ENABLE_OSMESA_CONTEXT)
  add_subdirectory(osmesa)
elseif(VTKm_ENABLE_EGL_CONTEXT)
  add_subdirectory(egl)
endif()
