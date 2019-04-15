##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
find_package(GLFW QUIET)
if(NOT TARGET GLFW::GLFW)
  return()
endif()

set(unit_tests
  UnitTestMapperGLFW.cxx
  )
vtkm_unit_tests(SOURCES ${unit_tests}
                TEST_ARGS -B
                LIBRARIES vtkm_rendering GLFW::GLFW)

