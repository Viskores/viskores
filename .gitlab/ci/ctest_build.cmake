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

if(NOT DEFINED ENV{GITLAB_CI_EMULATION})
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
    ctest_submit(PARTS Build BUILD_ID build_id)
    message(STATUS "Build submission build_id: ${build_id}")
  else()
    ctest_submit(PARTS Build)
  endif()
endif()

file(WRITE "${CTEST_BINARY_DIRECTORY}/compile_num_warnings.log" "${num_warnings}")

if (build_result)
  message(FATAL_ERROR "Failed to build")
endif ()
