##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================
function(add_benchmark name files)
  set(benchmarks )

  add_executable(${name}_SERIAL ${files})
  list(APPEND benchmarks ${name}_SERIAL)
  target_compile_definitions(${name}_SERIAL PRIVATE "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_SERIAL")

  if (TARGET vtkm::tbb)
    add_executable(${name}_TBB ${files})
    list(APPEND benchmarks ${name}_TBB)
    target_compile_definitions(${name}_TBB PRIVATE "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB")
  endif()

  if (TARGET vtkm::openmp)
    add_executable(${name}_OPENMP ${files})
    list(APPEND benchmarks ${name}_OPENMP)
    target_compile_definitions(${name}_OPENMP PRIVATE "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_OPENMP")
  endif()

  if (TARGET vtkm::cuda)
    get_filename_component(fname "${name}" NAME_WE)
    get_filename_component(fullpath "${name}.cxx" ABSOLUTE)
    file(GENERATE
          OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${fname}.cu
          CONTENT "#include \"${fullpath}\"")

    add_executable(${name}_CUDA ${CMAKE_CURRENT_BINARY_DIR}/${fname}.cu)
    list(APPEND benchmarks ${name}_CUDA)
    target_compile_definitions(${name}_CUDA PRIVATE "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA")
  endif()

  foreach(benchmark ${benchmarks})
    target_link_libraries(${benchmark} PRIVATE vtkm_cont)
    set_target_properties(${benchmark} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
      )
  endforeach()


endfunction()

set(benchmarks
  BenchmarkArrayTransfer
  BenchmarkCopySpeeds
  BenchmarkDeviceAdapter
  BenchmarkFieldAlgorithms
  BenchmarkTopologyAlgorithms
  )

foreach (benchmark ${benchmarks})
  add_benchmark(${benchmark} ${benchmark}.cxx)
endforeach ()

if(TARGET vtkm_rendering)
  add_benchmark(BenchmarkRayTracing BenchmarkRayTracing.cxx)
  target_link_libraries(BenchmarkRayTracing_SERIAL PRIVATE vtkm_rendering)
  if(TARGET BenchmarkRayTracing_TBB)
    target_link_libraries(BenchmarkRayTracing_TBB PRIVATE vtkm_rendering)
  endif()
  if(TARGET BenchmarkRayTracing_OPENMP)
    target_link_libraries(BenchmarkRayTracing_OPENMP PRIVATE vtkm_rendering)
  endif()
  if(TARGET BenchmarkRayTracing_CUDA)
    target_link_libraries(BenchmarkRayTracing_CUDA PRIVATE vtkm_rendering)
  endif()
endif()
