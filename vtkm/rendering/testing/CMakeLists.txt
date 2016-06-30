#============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 Sandia Corporation.
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  RenderTest.h
)

set(unit_tests
  UnitTestMapperRayTracer.cxx
  UnitTestMapperVolume.cxx
)

set(libs)
if (OPENGL_FOUND)
  list(APPEND libs ${OPENGL_LIBRARIES})
  if (EGL_FOUND)
    set(unit_tests ${unit_tests}
      UnitTestMapperEGL.cxx
      )
    list(APPEND libs ${EGL_LIBRARIES})    
  endif()
  #apple needs thes linker flags
  #-framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
  if (GLFW_FOUND)
    set(unit_tests ${unit_tests}
      UnitTestMapperGLFW.cxx
      )
    list(APPEND libs ${GLFW_LIBRARY})
    list(APPEND includes ${GLFW_INCLUDE_DIR})
  endif()

  if (GLUT_FOUND)
    set(unit_tests ${unit_tests}
      UnitTestMapperGLUT.cxx
      )
    list(APPEND libs ${GLUT_LIBRARY})
    list(APPEND includes ${GLUT_INCLUDE_DIR})
  endif()  
  
  if (OSMESA_FOUND)
    set(unit_tests ${unit_tests}
      UnitTestMapperOSMesa.cxx
      )
    list(APPEND libs ${OSMESA_LIBRARY})
  endif()
endif()

VTKM_unit_tests(SOURCES ${unit_tests}
                LIBRARIES ${libs})
