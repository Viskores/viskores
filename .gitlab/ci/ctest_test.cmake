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
  # placeholder for tests to exclude
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

if(NOT DEFINED ENV{GITLAB_CI_EMULATION})
  ctest_submit(PARTS Test BUILD_ID build_id)
  message(STATUS "Test submission build_id: ${build_id}")
endif()

if (test_result)
  #Current ctest return value only tracks if tests failed on the initial run.
  #So when we use repeat unit pass, and all tests now succede ctest will still
  #report a failure, making our gitlab-ci pipeline look red when it isn't
  #
  #To work around this issue we check if `Testing/Temporary/LastTestsFailed_*.log`
  #has a listing of tests that failed.
  set(testing_log_dir "$ENV{CI_PROJECT_DIR}/build/Testing/Temporary")
  file(GLOB tests_that_failed_log "${testing_log_dir}/LastTestsFailed_*.log")
 if(tests_that_failed_log)

    #Make sure the file has tests listed
    set(has_failing_tests true)
    file(STRINGS "${tests_that_failed_log}" failed_tests)
    list(LENGTH failed_tests length)
    if(length LESS_EQUAL 1)
      # each line looks like NUM:TEST_NAME
      string(FIND "${failed_tests}" ":" location)
      if(location EQUAL -1)
        #no ":" so no tests actually failed after all the re-runs
        set(has_failing_tests false)
      endif()
    endif()

    if(has_failing_tests)
      message(STATUS "Failing test from LastTestsFailed.log: \n ${failed_tests}")
      message(FATAL_ERROR "Failed to test")
    endif()
  endif()

endif ()
