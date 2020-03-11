##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##=============================================================================

cmake_minimum_required(VERSION 3.8)

# Read the files from the build directory that contain
# host information ( name, parallel level, etc )
include("$ENV{CI_PROJECT_DIR}/build/CIState.cmake")
ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")


# Pick up from where the configure left off.
ctest_start(APPEND)
message(STATUS "CTEST_BUILD_FLAGS: ${CTEST_BUILD_FLAGS}")
ctest_build(APPEND
  NUMBER_WARNINGS num_warnings
  RETURN_VALUE build_result)
ctest_submit(PARTS Build)

if (build_result)
  message(FATAL_ERROR
    "Failed to build")
endif ()
