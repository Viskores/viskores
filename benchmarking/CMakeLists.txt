##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#Find Google Benchmark.Note that benchmark_DIR must be pointed at an
#installation, not a build directory.
find_package(benchmark REQUIRED)

function(add_benchmark)
  set(options)
  set(oneValueArgs NAME FILE)
  set(multiValueArgs LIBS)
  cmake_parse_arguments(VTKm_AB
          "${options}" "${oneValueArgs}" "${multiValueArgs}"
          ${ARGN}
          )
  set(exe_name ${VTKm_AB_NAME})

  add_executable(${exe_name} ${VTKm_AB_FILE})
  target_link_libraries(${exe_name} PRIVATE ${VTKm_AB_LIBS})
  target_link_libraries(${exe_name} PRIVATE benchmark::benchmark)
  vtkm_add_drop_unused_function_flags(${exe_name})
  vtkm_add_target_information(${exe_name})

  set_target_properties(${exe_name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
    CXX_VISIBILITY_PRESET "hidden"
    CUDA_VISIBILITY_PRESET "hidden"
  )

  vtkm_add_target_information(${exe_name} DEVICE_SOURCES ${VTKm_AB_FILE})
endfunction()

set(benchmarks
  BenchmarkArrayTransfer
  BenchmarkAtomicArray
  BenchmarkCopySpeeds
  BenchmarkDeviceAdapter
  BenchmarkFieldAlgorithms
  BenchmarkFilters
  BenchmarkODEIntegrators
  BenchmarkTopologyAlgorithms
  )

#Taking too long to compile with HIPCC
if(HIP IN_LIST Kokkos_DEVICES)
  list(REMOVE_ITEM benchmarks
       BenchmarkDeviceAdapter
       BenchmarkODEIntegrators
      )
endif()

set(VTKm_BENCHS_RANGE_LOWER_BOUNDARY 4096 CACHE STRING "Smallest sample for input size bench for BenchmarkDeviceAdapter")
set(VTKm_BENCHS_RANGE_UPPER_BOUNDARY 134217728 CACHE STRING "Biggest sample for input size bench for BenchmarkDeviceAdapter")
mark_as_advanced(VTKm_BENCHS_RANGE_LOWER_BOUNDARY VTKm_BENCHS_RANGE_UPPER_BOUNDARY)

foreach (benchmark ${benchmarks})
  add_benchmark(NAME ${benchmark} FILE ${benchmark}.cxx LIBS vtkm_source vtkm_filter vtkm_io)
endforeach ()

if(NOT HIP IN_LIST Kokkos_DEVICES)
  target_compile_definitions(BenchmarkDeviceAdapter PUBLIC VTKm_BENCHS_RANGE_LOWER_BOUNDARY=${VTKm_BENCHS_RANGE_LOWER_BOUNDARY})
  target_compile_definitions(BenchmarkDeviceAdapter PUBLIC VTKm_BENCHS_RANGE_UPPER_BOUNDARY=${VTKm_BENCHS_RANGE_UPPER_BOUNDARY})
endif()

if(TARGET vtkm_rendering)
  add_benchmark(NAME BenchmarkRayTracing FILE BenchmarkRayTracing.cxx LIBS vtkm_rendering vtkm_source)
  add_benchmark(NAME BenchmarkInSitu FILE BenchmarkInSitu.cxx LIBS vtkm_rendering vtkm_source vtkm_filter vtkm_io)
endif()
