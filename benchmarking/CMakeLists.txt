##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
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
  set_target_properties(${exe_name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
    )

  if (TARGET vtkm::cuda)
    set_source_files_properties(${VTKm_AB_FILE} PROPERTIES LANGUAGE "CUDA")
    set_property(TARGET ${exe_name} PROPERTY CUDA_SEPARABLE_COMPILATION ON)
  endif()
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
  add_benchmark(NAME ${benchmark} FILE ${benchmark}.cxx LIBS vtkm_filter vtkm_cont)
endforeach ()

if(TARGET vtkm_rendering)
  add_benchmark(NAME BenchmarkRayTracing FILE BenchmarkRayTracing.cxx LIBS vtkm_rendering)
endif()
