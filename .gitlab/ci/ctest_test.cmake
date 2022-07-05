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

# We need this CMake versions for tests
cmake_minimum_required(VERSION 3.18)

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

string(REPLACE " " ";" test_exclusions "${test_exclusions}")
string(REPLACE ";" "|" test_exclusions "${test_exclusions}")
if (test_exclusions)
  set(test_exclusions EXCLUDE "(${test_exclusions})")
endif ()

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
  set(junit_args OUTPUT_JUNIT "${CTEST_BINARY_DIRECTORY}/junit.xml")
endif()

set(PARALLEL_LEVEL "10")
if (DEFINED ENV{CTEST_MAX_PARALLELISM})
  set(PARALLEL_LEVEL $ENV{CTEST_MAX_PARALLELISM})
endif()

if (DEFINED ENV{TEST_INCLUSIONS})
  set(test_inclusions INCLUDE $ENV{TEST_INCLUSIONS})
  unset(test_exclusions)
endif()


ctest_test(APPEND
  PARALLEL_LEVEL ${PARALLEL_LEVEL}
  RETURN_VALUE test_result
  ${test_exclusions}
  ${test_inclusions}
  REPEAT "UNTIL_PASS:${CTEST_REPEAT_UNTIL_PASS}"
  ${junit_args}
  )
  message(STATUS "ctest_test RETURN_VALUE: ${test_result}")

if(VTKm_ENABLE_PERFORMANCE_TESTING)
  file(GLOB note_files
       "${CTEST_BINARY_DIRECTORY}/benchmark_*.stdout"
       "${CTEST_BINARY_DIRECTORY}/compare_*.stdout"
       "${CTEST_BINARY_DIRECTORY}/$ENV{CI_COMMIT_SHA}_*.json")
  list(APPEND CTEST_NOTES_FILES ${note_files})
endif()

if(NOT DEFINED ENV{GITLAB_CI_EMULATION})
  ctest_submit(PARTS Test Notes BUILD_ID build_id)
  message(STATUS "Test submission build_id: ${build_id}")
endif()

if (test_result)
  message(FATAL_ERROR "Failed to test")
endif ()
