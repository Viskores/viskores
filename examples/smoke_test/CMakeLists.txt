##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

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
project(ViskoresSmokeTest CXX)
include(CTest)

find_package(Viskores REQUIRED)

if(TARGET viskores::source)
  add_executable(smoke_test smoke_test.cxx)
  target_link_libraries(smoke_test PRIVATE viskores::source)

  # Only add this test when this a standalone project
  if (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    add_test(NAME SmokeTestInternal COMMAND ${CMAKE_BINARY_DIR}/smoke_test)
  endif()
endif()
