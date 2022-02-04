##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(VTKmSmokeTest CXX)
include(CTest)

find_package(VTKm REQUIRED)

if (VTKm_VERSION VERSION_LESS 1.7)
  message(FATAL_ERROR "Requires VTK-m 1.7 or newer")
endif()

add_executable(smoke_test smoke_test.cxx)
target_link_libraries(smoke_test PRIVATE vtkm_source)

# Only add this test when this an standalone project
if (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  add_test(NAME SmokeTestInternal COMMAND ${CMAKE_BINARY_DIR}/smoke_test)
endif()
