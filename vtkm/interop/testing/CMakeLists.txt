##============================================================================
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
  TestingOpenGLInterop.h
  TestingTransferFancyHandles.h
  )
vtkm_declare_headers(${headers} TESTABLE FALSE)


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
  vtkm_unit_tests(SOURCES ${unit_tests} LIBRARIES vtkm_rendering GLUT::GLUT)
endif()
