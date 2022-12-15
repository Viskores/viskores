##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

include("${VTKm_SOURCE_DIR}/CMake/testing/VTKmPerformanceTestLib.cmake")

REQUIRE_FLAG("VTKm_PERF_NAME")
REQUIRE_FLAG("VTKm_PERF_COMPARE_JSON")
REQUIRE_FLAG("VTKm_PERF_COMPARE_STDOUT")

REQUIRE_FLAG_MUTABLE("VTKm_PERF_REPO")
REQUIRE_FLAG_MUTABLE("VTKm_PERF_ALPHA")
REQUIRE_FLAG_MUTABLE("VTKm_PERF_DIST")

###FIND MOST RECENT BASELINE####################################################
execute(COMMAND /usr/bin/git -C "${VTKm_SOURCE_DIR}" merge-base origin/master @
        OUTPUT_VARIABLE GIT_BASE_COMMIT)

string(STRIP "${GIT_BASE_COMMIT}" GIT_BASE_COMMIT)

execute_process(COMMAND /usr/bin/git -C "${VTKm_SOURCE_DIR}" log --format=%H --first-parent "${GIT_BASE_COMMIT}"
                OUTPUT_VARIABLE GIT_ANCESTOR_COMMITS
                COMMAND_ECHO STDOUT
                ECHO_ERROR_VARIABLE
                COMMAND_ERROR_IS_FATAL ANY
                )

string(REPLACE "\n" ";" GIT_ANCESTOR_COMMITS ${GIT_ANCESTOR_COMMITS})

foreach(commit IN LISTS GIT_ANCESTOR_COMMITS)
  if (EXISTS "${VTKm_PERF_REPO}/${commit}_${VTKm_PERF_NAME}.json")
    set(BASELINE_REPORT "${VTKm_PERF_REPO}/${commit}_${VTKm_PERF_NAME}.json")
    break()
  endif()
endforeach()

if (NOT DEFINED BASELINE_REPORT)
  message(FATAL_ERROR "PerformanceTestReport: no ancestor benchmarks record found")
endif()

###RUN COMPARE_PY SCRIPT########################################################
execute(COMMAND /usr/bin/python3
  "${VTKm_SOURCE_DIR}/Utilities/Scripts/compare.py"
  --alpha "${VTKm_PERF_ALPHA}"
  --dist "${VTKm_PERF_DIST}"
  benchmarks "${BASELINE_REPORT}" "${VTKm_PERF_COMPARE_JSON}"
  OUTPUT_VARIABLE compare_py_output
  )

# Write compare.py output to disk
file(WRITE "${VTKm_PERF_COMPARE_STDOUT}" "${compare_py_output}")

###PERFORM NULL HYPHOTESIS######################################################
string(REGEX MATCHALL "[^\n]*time_pvalue[^\n]*" pvalues_list ${compare_py_output})

foreach(pvalue_line IN LISTS pvalues_list)
  # We only take into consideration the wall time of the test
  string(REGEX MATCH "(.*)/manual_time_pvalue[ \t]+([0-9.]+)[ ]+" ignoreme ${pvalue_line})
  if (CMAKE_MATCH_0)
    set(benchmark_name "${CMAKE_MATCH_1}")
    set(benchmark_pvalue "${CMAKE_MATCH_2}")
    if("${benchmark_pvalue}" LESS "${VTKm_PERF_ALPHA}")
      list(APPEND time_failed_benchs ${benchmark_name})
    endif()
  endif()
endforeach()

if(time_failed_benchs)
  string(REPLACE ";" "\n" time_failed_benchs "${time_failed_benchs}")
  message(FATAL_ERROR "Time-failed Benchmarks:\n${time_failed_benchs}")
endif()
