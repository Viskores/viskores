##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

# Find Google Benchmark. Note that benchmark_DIR must be pointed at an
# installation, not a build directory.
find_package(benchmark)

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
  BenchmarkTopologyAlgorithms
  )

foreach (benchmark ${benchmarks})
  add_benchmark(NAME ${benchmark} FILE ${benchmark}.cxx LIBS vtkm_source vtkm_filter)
endforeach ()

if(TARGET vtkm_rendering)
  add_benchmark(NAME BenchmarkRayTracing FILE BenchmarkRayTracing.cxx LIBS vtkm_rendering)
endif()
