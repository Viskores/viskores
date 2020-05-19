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

set(test_exclusions
  # placeholder for tests to exclude provided by the env
  $ENV{CTEST_EXCLUSIONS}
)

string(REPLACE ";" "|" test_exclusions "${test_exclusions}")
if (test_exclusions)
  set(test_exclusions "(${test_exclusions})")
endif ()

ctest_test(APPEND
  PARALLEL_LEVEL "10"
  RETURN_VALUE test_result
  EXCLUDE "${test_exclusions}"
  REPEAT "UNTIL_PASS:3"
  )
  message(STATUS "ctest_test RETURN_VALUE: ${test_result}")

if(NOT DEFINED ENV{GITLAB_CI_EMULATION})
  ctest_submit(PARTS Test BUILD_ID build_id)
  message(STATUS "Test submission build_id: ${build_id}")
endif()

if (test_result)
  message(FATAL_ERROR "Failed to test")
endif ()
