##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
if(NOT TARGET GLUT::GLUT)
  find_package(GLUT QUIET)

  if(NOT TARGET GLUT::GLUT)
    return()
  endif()
endif()

set(unit_tests
  UnitTestMapperGLUT.cxx
  )

vtkm_unit_tests(SOURCES ${unit_tests}
                TEST_ARGS -B
                LIBRARIES vtkm_rendering GLUT::GLUT)

