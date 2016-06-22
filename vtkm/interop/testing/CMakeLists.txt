##============================================================================
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
  TestingOpenGLInterop.h
  TestingWindow.h
  WindowBase.h
  )

set(unit_tests
  UnitTestTransferToOpenGL.cxx
  )

# Need glut for these tests
if(GLUT_FOUND)
  set(VTKm_INCLUDE_DIRS ${VTKm_INCLUDE_DIRS} ${GLUT_INCLUDE_DIR})
  set(VTKM_LIBRARIES ${VTKm_LIBRARIES} ${GLUT_LIBRARIES})

  vtkm_declare_headers(${headers})

  vtkm_unit_tests(SOURCES ${unit_tests})
else()
  message(STATUS "Interop tests disabled. They need GLUT.")
endif()
