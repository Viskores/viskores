##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

if(NOT vtkm_module_current STREQUAL "benchmarking")
  message(FATAL_ERROR "Benchmarking CMakeLists.txt not loaded by benchmarking module.")
endif()

#Find Google Benchmark.Note that benchmark_DIR must be pointed at an
#installation, not a build directory.
find_package(benchmark REQUIRED)

function(add_benchmark exe_name)
  add_executable(${exe_name} ${exe_name}.cxx)
  target_link_libraries(${exe_name} PRIVATE benchmark::benchmark)
  vtkm_module_get_property(depends benchmarking DEPENDS)
  target_link_libraries(${exe_name} PRIVATE ${depends})
  vtkm_module_get_property(optional_depends benchmarking OPTIONAL_DEPENDS)
  foreach(dep IN LISTS optional_depends)
    if(TARGET ${dep})
      target_link_libraries(${exe_name} PRIVATE ${dep})
    endif()
  endforeach()
  vtkm_add_drop_unused_function_flags(${exe_name})
  vtkm_add_target_information(${exe_name})

  set_target_properties(${exe_name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
    CXX_VISIBILITY_PRESET "hidden"
    CUDA_VISIBILITY_PRESET "hidden"
  )

  # At some point, we might not want to compile benchmarks with device compilers.
  vtkm_add_target_information(${exe_name} DEVICE_SOURCES ${exe_name}.cxx)
endfunction()

set(benchmarks
  BenchmarkArrayTransfer
  BenchmarkAtomicArray
  BenchmarkCopySpeeds
  BenchmarkDeviceAdapter
  BenchmarkFieldAlgorithms
  BenchmarkFilters
  BenchmarkLocators
  BenchmarkODEIntegrators
  BenchmarkTopologyAlgorithms
  )

if(TARGET vtkm_rendering)
  list(APPEND benchmarks
    BenchmarkRayTracing
    BenchmarkInSitu
    )
endif()

set(VTKm_BENCHS_RANGE_LOWER_BOUNDARY 4096 CACHE STRING "Smallest sample for input size bench for BenchmarkDeviceAdapter")
set(VTKm_BENCHS_RANGE_UPPER_BOUNDARY 134217728 CACHE STRING "Biggest sample for input size bench for BenchmarkDeviceAdapter")
mark_as_advanced(VTKm_BENCHS_RANGE_LOWER_BOUNDARY VTKm_BENCHS_RANGE_UPPER_BOUNDARY)

foreach (benchmark ${benchmarks})
  add_benchmark(${benchmark})
endforeach ()

target_compile_definitions(BenchmarkDeviceAdapter PUBLIC VTKm_BENCHS_RANGE_LOWER_BOUNDARY=${VTKm_BENCHS_RANGE_LOWER_BOUNDARY})
target_compile_definitions(BenchmarkDeviceAdapter PUBLIC VTKm_BENCHS_RANGE_UPPER_BOUNDARY=${VTKm_BENCHS_RANGE_UPPER_BOUNDARY})

if(VTKm_ENABLE_PERFORMANCE_TESTING)
  include("${VTKm_SOURCE_DIR}/CMake/testing/VTKmPerformanceTest.cmake")
  add_benchmark_test(BenchmarkFilters
    NAME BenchThreshold
    REGEX BenchThreshold
    )
  add_benchmark_test(BenchmarkFilters
    NAME BenchTetrahedralize
    REGEX BenchTetrahedralize
    )
  add_benchmark_test(BenchmarkFilters
    NAME BenchContourStructured
    REGEX
      BenchContour/IsStructuredDataSet:1/NIsoVals:12/MergePts:1/GenNormals:0.*/MultiPartitioned:0
      BenchContour/IsStructuredDataSet:1/NIsoVals:12/MergePts:0/GenNormals:1/FastNormals:1.*/MultiPartitioned:0
    )
  add_benchmark_test(BenchmarkFilters
    NAME BenchContourUnstructured
    REGEX
      BenchContour/IsStructuredDataSet:0/NIsoVals:12/MergePts:1/GenNormals:0.*/MultiPartitioned:0
      BenchContour/IsStructuredDataSet:0/NIsoVals:12/MergePts:0/GenNormals:1/FastNormals:1.*/MultiPartitioned:0
    )
  add_benchmark_test(BenchmarkFilters
    NAME BenchContourPartitioned
    ARGS --wavelet-dim=32 --num-partitions=128
    REGEX
      BenchContour/IsStructuredDataSet:1/NIsoVals:12/MergePts:1/GenNormals:0.*/MultiPartitioned:1
      BenchContour/IsStructuredDataSet:1/NIsoVals:12/MergePts:0/GenNormals:1/FastNormals:1.*/MultiPartitioned:1
      BenchContour/IsStructuredDataSet:0/NIsoVals:12/MergePts:1/GenNormals:0.*/MultiPartitioned:1
      BenchContour/IsStructuredDataSet:0/NIsoVals:12/MergePts:0/GenNormals:1/FastNormals:1.*/MultiPartitioned:1
    )
  add_benchmark_test(BenchmarkFilters
    NAME BenchVertexClustering
    REGEX BenchVertexClustering/NumDivs:256
    )
  if(TARGET vtkm_rendering)
    add_benchmark_test(BenchmarkInSitu REGEX "BenchContour")
  endif()
endif()
