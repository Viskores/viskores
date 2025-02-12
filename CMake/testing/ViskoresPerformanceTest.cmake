##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#-----------------------------------------------------------------------------
# Adds a performance benchmark test
#
# Usage:
#   add_benchmark_test(benchmark
#     [ NAME <name> ]
#     [ ARGS <args...> ]
#     [ REGEX <benchmark_regex...> ]
#     )
#
# benchmark:    Target of an executable that uses Google Benchmark.
#
# NAME:         The name given to the CMake tests. The benchmark target name is used
#               if NAME is not specified.
#
# ARGS:         Extra arguments passed to the benchmark executable when run.
#
# REGEX:        Regular expressions that select the specific benchmarks within the binary
#               to be used. It populates the Google Benchmark
#               --benchmark_filter parameter. When multiple regexes are passed
#               as independent positional arguments, they are joined using the "|"
#               regex operator before populating the  `--benchmark_filter` parameter.
#
function(add_benchmark_test benchmark)

  # We need JSON support among other things for this to work
  if (CMAKE_VERSION VERSION_LESS 3.19)
    message(FATAL_ERROR "Performance regression testing needs CMAKE >= 3.19")
    return()
  endif()

  ###TEST VARIABLES############################################################

  set(options)
  set(one_value_keywords NAME)
  set(multi_value_keywords ARGS REGEX)
  cmake_parse_arguments(PARSE_ARGV 1 Viskores_PERF "${options}" "${one_value_keywords}" "${multi_value_keywords}")
  if (Viskores_PERF_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Bad arguments to add_benchmark_test (${Viskores_PERF_UNPARSED_ARGUMENTS}).")
  endif()

  if (NOT Viskores_PERF_NAME)
    set(Viskores_PERF_NAME ${benchmark})
  endif()

  if (Viskores_PERF_REGEX)
    string(REPLACE ";" "|" Viskores_PERF_REGEX "${Viskores_PERF_REGEX}")
  else()
    set(Viskores_PERF_REGEX ".*")
  endif()

  set(Viskores_PERF_REMOTE_URL "https://gitlab.kitware.com/vbolea/viskores-benchmark-records.git")

  # Parameters for the null hypothesis test
  set(Viskores_PERF_ALPHA  0.05)
  set(Viskores_PERF_REPETITIONS 10)
  set(Viskores_PERF_MIN_TIME 1)
  set(Viskores_PERF_DIST "normal")

  set(Viskores_PERF_REPO           "${CMAKE_BINARY_DIR}/viskores-benchmark-records")
  set(Viskores_PERF_COMPARE_JSON   "${CMAKE_BINARY_DIR}/nocommit_${Viskores_PERF_NAME}.json")
  set(Viskores_PERF_STDOUT         "${CMAKE_BINARY_DIR}/benchmark_${Viskores_PERF_NAME}.stdout")
  set(Viskores_PERF_COMPARE_STDOUT "${CMAKE_BINARY_DIR}/compare_${Viskores_PERF_NAME}.stdout")

  if (DEFINED ENV{CI_COMMIT_SHA})
    set(Viskores_PERF_COMPARE_JSON "${CMAKE_BINARY_DIR}/$ENV{CI_COMMIT_SHA}_${Viskores_PERF_NAME}.json")
  endif()

  # Only upload when we are inside a CI build and in master.  We need to check
  # if VISKORES_BENCH_RECORDS_TOKEN is either defined or non-empty, the reason is
  # that in Gitlab CI Variables for protected branches are also defined in MR
  # from forks, however, they are empty.
  if (DEFINED ENV{VISKORES_BENCH_RECORDS_TOKEN} AND NOT $ENV{VISKORES_BENCH_RECORDS_TOKEN} STREQUAL "")
    set(enable_upload TRUE)
  endif()

  set(test_name "PerformanceTest${Viskores_PERF_NAME}")

  ###TEST INVOKATIONS##########################################################
  if (NOT TEST PerformanceTestFetch)
    add_test(NAME "PerformanceTestFetch"
      COMMAND ${CMAKE_COMMAND}
      "-DViskores_PERF_REPO=${Viskores_PERF_REPO}"
      "-DViskores_SOURCE_DIR=${Viskores_SOURCE_DIR}"
      "-DViskores_PERF_REMOTE_URL=${Viskores_PERF_REMOTE_URL}"
      -P "${Viskores_SOURCE_DIR}/CMake/testing/ViskoresPerformanceTestFetch.cmake"
      )
    set_property(TEST PerformanceTestFetch PROPERTY FIXTURES_SETUP "FixturePerformanceTestSetup")
  endif()

  add_test(NAME "${test_name}Run"
    COMMAND ${CMAKE_COMMAND}
    "-DViskores_PERF_BENCH_DEVICE=Any"
    "-DViskores_PERF_BENCH_PATH=${CMAKE_BINARY_DIR}/bin/${benchmark}"
    "-DViskores_PERF_ARGS=${Viskores_PERF_ARGS}"
    "-DViskores_PERF_REGEX=${Viskores_PERF_REGEX}"
    "-DViskores_PERF_REPETITIONS=${Viskores_PERF_REPETITIONS}"
    "-DViskores_PERF_MIN_TIME=${Viskores_PERF_MIN_TIME}"
    "-DViskores_PERF_COMPARE_JSON=${Viskores_PERF_COMPARE_JSON}"
    "-DViskores_PERF_STDOUT=${Viskores_PERF_STDOUT}"
    "-DViskores_SOURCE_DIR=${Viskores_SOURCE_DIR}"
    -P "${Viskores_SOURCE_DIR}/CMake/testing/ViskoresPerformanceTestRun.cmake"
    )

  add_test(NAME "${test_name}Report"
    COMMAND ${CMAKE_COMMAND}
    "-DViskores_BINARY_DIR=${Viskores_BINARY_DIR}"
    "-DViskores_PERF_ALPHA=${Viskores_PERF_ALPHA}"
    "-DViskores_PERF_COMPARE_JSON=${Viskores_PERF_COMPARE_JSON}"
    "-DViskores_PERF_COMPARE_STDOUT=${Viskores_PERF_COMPARE_STDOUT}"
    "-DViskores_PERF_DIST=${Viskores_PERF_DIST}"
    "-DViskores_PERF_NAME=${Viskores_PERF_NAME}"
    "-DViskores_PERF_REPO=${Viskores_PERF_REPO}"
    "-DViskores_SOURCE_DIR=${Viskores_SOURCE_DIR}"
    -P "${Viskores_SOURCE_DIR}/CMake/testing/ViskoresPerformanceTestReport.cmake"
    )

  if (enable_upload)
    add_test(NAME "${test_name}Upload"
      COMMAND ${CMAKE_COMMAND}
      "-DViskores_PERF_REPO=${Viskores_PERF_REPO}"
      "-DViskores_PERF_COMPARE_JSON=${Viskores_PERF_COMPARE_JSON}"
      "-DViskores_SOURCE_DIR=${Viskores_SOURCE_DIR}"
      -P "${Viskores_SOURCE_DIR}/CMake/testing/ViskoresPerformanceTestUpload.cmake"
      )

    set_tests_properties("${test_name}Upload" PROPERTIES
      DEPENDS ${test_name}Report
      FIXTURES_REQUIRED "FixturePerformanceTestCleanUp"
      REQUIRED_FILES "${Viskores_PERF_COMPARE_JSON}"
      RUN_SERIAL ON)
  endif()

  ###TEST PROPERTIES###########################################################
  set_property(TEST ${test_name}Report PROPERTY DEPENDS ${test_name}Run)
  set_property(TEST ${test_name}Report PROPERTY FIXTURES_REQUIRED "FixturePerformanceTestSetup")

  set_tests_properties("${test_name}Report"
    PROPERTIES
    REQUIRED_FILES "${Viskores_PERF_COMPARE_JSON}")

  set_tests_properties("${test_name}Run"
                        "${test_name}Report"
                        "PerformanceTestFetch"
                        PROPERTIES RUN_SERIAL ON)

  set_tests_properties(${test_name}Run PROPERTIES TIMEOUT 1800)
endfunction()
