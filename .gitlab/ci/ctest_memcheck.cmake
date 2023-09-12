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

if(NOT CTEST_MEMORYCHECK_TYPE)
  set(CTEST_MEMORYCHECK_TYPE "$ENV{CTEST_MEMORYCHECK_TYPE}")
endif()

if(NOT CTEST_MEMORYCHECK_SANITIZER_OPTIONS)
  set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS "$ENV{CTEST_MEMORYCHECK_SANITIZER_OPTIONS}")
endif()

if(NOT CTEST_MEMORYCHECK_SUPPRESSIONS_FILE)
  if(CTEST_MEMORYCHECK_TYPE STREQUAL "LeakSanitizer")
    set(CTEST_MEMORYCHECK_SUPPRESSIONS_FILE "${CTEST_SOURCE_DIRECTORY}/CMake/testing/lsan.supp")
  endif()
endif()

set(test_exclusions
  # placeholder for tests to exclude provided by the env
  $ENV{CTEST_EXCLUSIONS}
)

string(REPLACE " " ";" test_exclusions "${test_exclusions}")
string(REPLACE ";" "|" test_exclusions "${test_exclusions}")
if (test_exclusions)
  set(test_exclusions EXCLUDE "(${test_exclusions})")
endif ()

if (DEFINED ENV{TEST_INCLUSIONS})
  set(test_inclusions INCLUDE $ENV{TEST_INCLUSIONS})
  unset(test_exclusions)
endif()

set(PARALLEL_LEVEL "10")
if (DEFINED ENV{CTEST_MAX_PARALLELISM})
  set(PARALLEL_LEVEL $ENV{CTEST_MAX_PARALLELISM})
endif()

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
  set(junit_args OUTPUT_JUNIT "${CTEST_BINARY_DIRECTORY}/junit.xml")
endif()

# reduced parallel level so we don't exhaust system resources
ctest_memcheck(
  PARALLEL_LEVEL ${PARALLEL_LEVEL}
  RETURN_VALUE test_result
  ${test_exclusions}
  ${test_inclusions}
  DEFECT_COUNT defects
  ${junit_args}
  )

ctest_submit(PARTS Memcheck BUILD_ID build_id)
  message(STATUS "Memcheck submission build_id: ${build_id}")

if (defects)
  message(FATAL_ERROR "Found ${defects} memcheck defects")
endif ()


if (test_result)
  message(FATAL_ERROR "Failed to test")
endif ()
