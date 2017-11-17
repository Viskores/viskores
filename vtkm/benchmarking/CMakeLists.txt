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

set(benchmarks
  BenchmarkArrayTransfer
  BenchmarkCopySpeeds
  BenchmarkDeviceAdapter
  BenchmarkFieldAlgorithms
  BenchmarkTopologyAlgorithms
  )

#set(benchmark_headers
#  Benchmarker.h
#  )

function(add_benchmark name files)
  add_executable(${name}_SERIAL ${files})
  target_link_libraries(${name}_SERIAL PRIVATE vtkm_cont)

  if (TARGET vtkm::tbb)
    add_executable(${name}_TBB ${files})
    target_link_libraries(${name}_TBB PRIVATE vtkm_cont)
  endif()

  if (TARGET vtkm::cuda)
    set_source_files_properties(${files} PROPERTIES LANGUAGE "CUDA")
    add_executable(${name}_CUDA ${files})
    target_link_libraries(${name}_CUDA PRIVATE vtkm_cont)
  endif()
endfunction()

foreach (benchmark ${benchmarks})
  add_benchmark(${benchmark} ${benchmark}.cxx)
endforeach ()
