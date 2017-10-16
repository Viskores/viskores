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

set(benchmark_srcs
  BenchmarkArrayTransfer.cxx
  BenchmarkCopySpeeds.cxx
  BenchmarkDeviceAdapter.cxx
  BenchmarkFieldAlgorithms.cxx
  BenchmarkTopologyAlgorithms.cxx
  )

set(benchmark_headers
  Benchmarker.h
  )

vtkm_save_benchmarks(${benchmark_srcs}
  HEADERS ${benchmark_headers}
  )

vtkm_benchmarks(VTKM_DEVICE_ADAPTER_SERIAL)

if (VTKm_ENABLE_CUDA)
  vtkm_benchmarks(VTKM_DEVICE_ADAPTER_CUDA)
endif()
if (VTKm_ENABLE_TBB)
  vtkm_benchmarks(VTKM_DEVICE_ADAPTER_TBB)
endif()

