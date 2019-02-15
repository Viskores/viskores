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
function(add_benchmark)
  set(options)
  set(oneValueArgs NAME FILE)
  set(multiValueArgs LIBS)
  cmake_parse_arguments(VTKm_AB
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )
  set(exe_name ${VTKm_AB_NAME})
  if (TARGET vtkm::cuda)
    get_filename_component(fname ${VTKm_AB_FILE} NAME_WE)
    get_filename_component(fullpath ${VTKm_AB_FILE} ABSOLUTE)
    file(GENERATE
          OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${fname}.cu
          CONTENT "#include \"${fullpath}\"")

    add_executable(${exe_name} ${CMAKE_CURRENT_BINARY_DIR}/${fname}.cu)
    set_property(TARGET ${exe_name} PROPERTY CUDA_SEPARABLE_COMPILATION ON)
  else()
    add_executable(${exe_name} ${VTKm_AB_FILE})
  endif()

  target_link_libraries(${exe_name} PRIVATE ${VTKm_AB_LIBS})
  set_target_properties(${exe_name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
    )


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
