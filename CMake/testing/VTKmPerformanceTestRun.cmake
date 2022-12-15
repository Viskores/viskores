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

REQUIRE_FLAG("VTKm_PERF_BENCH_PATH")
REQUIRE_FLAG("VTKm_PERF_REGEX")
REQUIRE_FLAG("VTKm_PERF_COMPARE_JSON")
REQUIRE_FLAG("VTKm_PERF_STDOUT")

REQUIRE_FLAG_MUTABLE("VTKm_PERF_BENCH_DEVICE")
REQUIRE_FLAG_MUTABLE("VTKm_PERF_REPETITIONS")
REQUIRE_FLAG_MUTABLE("VTKm_PERF_MIN_TIME")

execute(
  COMMAND "${VTKm_PERF_BENCH_PATH}"
  --vtkm-device "${VTKm_PERF_BENCH_DEVICE}"
  ${VTKm_PERF_ARGS}
  "--benchmark_filter=${VTKm_PERF_REGEX}"
  "--benchmark_out=${VTKm_PERF_COMPARE_JSON}"
  "--benchmark_repetitions=${VTKm_PERF_REPETITIONS}"
  "--benchmark_min_time=${VTKm_PERF_MIN_TIME}"
  --benchmark_out_format=json
  --benchmark_counters_tabular=true
  OUTPUT_VARIABLE report_output
  )

# Write compare.py output to disk
file(WRITE "${VTKm_PERF_STDOUT}" "${report_output}")
