##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
cmake_minimum_required(VERSION 3.3...3.12 FATAL_ERROR)
project(Histogram CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
if (VTKm_ENABLE_MPI)
  add_executable(Histogram_SERIAL Histogram.cxx HistogramMPI.h HistogramMPI.hxx)
  target_compile_definitions(Histogram_SERIAL PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_SERIAL")
  target_link_libraries(Histogram_SERIAL PRIVATE vtkm_cont)

  if(TARGET vtkm::tbb)
    add_executable(Histogram_TBB Histogram.cxx HistogramMPI.h HistogramMPI.hxx)
    target_compile_definitions(Histogram_TBB PRIVATE
                               "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB")
    target_link_libraries(Histogram_TBB PRIVATE vtkm_cont)
  endif()

  if(TARGET vtkm::cuda)
    vtkm_compile_as_cuda(cudaSource Histogram.cxx)
    add_executable(Histogram_CUDA ${cudaSource} HistogramMPI.h HistogramMPI.hxx)

    target_compile_definitions(Histogram_CUDA PRIVATE
                               "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA")
    target_link_libraries(Histogram_CUDA PRIVATE vtkm_cont)
  endif()
endif()
