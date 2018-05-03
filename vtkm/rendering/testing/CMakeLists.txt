#============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  RenderTest.h
  )

vtkm_declare_headers(${headers})

set(unit_tests
  UnitTestCanvas.cxx
  UnitTestMapperConnectivity.cxx
  UnitTestMultiMapper.cxx
  UnitTestMapperRayTracer.cxx
  UnitTestMapperWireframer.cxx
  UnitTestMapperVolume.cxx
)

vtkm_unit_tests(NAME Rendering BACKEND SERIAL SOURCES ${unit_tests} LIBRARIES vtkm_rendering)
if (TARGET vtkm::tbb)
  vtkm_unit_tests(NAME Rendering BACKEND TBB SOURCES ${unit_tests} LIBRARIES vtkm_rendering)
endif()
if (TARGET vtkm::cuda)
  vtkm_unit_tests(NAME Rendering BACKEND CUDA SOURCES ${unit_tests} LIBRARIES vtkm_rendering)
endif()


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
