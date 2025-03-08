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

include("${Viskores_SOURCE_DIR}/CMake/testing/ViskoresPerformanceTestLib.cmake")

REQUIRE_FLAG("Viskores_PERF_BENCH_PATH")
REQUIRE_FLAG("Viskores_PERF_REGEX")
REQUIRE_FLAG("Viskores_PERF_COMPARE_JSON")
REQUIRE_FLAG("Viskores_PERF_STDOUT")

REQUIRE_FLAG_MUTABLE("Viskores_PERF_BENCH_DEVICE")
REQUIRE_FLAG_MUTABLE("Viskores_PERF_REPETITIONS")
REQUIRE_FLAG_MUTABLE("Viskores_PERF_MIN_TIME")

execute(
  COMMAND "${Viskores_PERF_BENCH_PATH}"
  --viskores-device "${Viskores_PERF_BENCH_DEVICE}"
  ${Viskores_PERF_ARGS}
  "--benchmark_filter=${Viskores_PERF_REGEX}"
  "--benchmark_out=${Viskores_PERF_COMPARE_JSON}"
  "--benchmark_repetitions=${Viskores_PERF_REPETITIONS}"
  "--benchmark_min_time=${Viskores_PERF_MIN_TIME}"
  --benchmark_out_format=json
  --benchmark_counters_tabular=true
  OUTPUT_VARIABLE report_output
  )

# Write compare.py output to disk
file(WRITE "${Viskores_PERF_STDOUT}" "${report_output}")
