##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION 3.8...3.15 FATAL_ERROR)
project(Histogram CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
if (VTKm_ENABLE_MPI)
  add_executable(Histogram Histogram.cxx HistogramMPI.h HistogramMPI.hxx)
  target_link_libraries(Histogram PRIVATE vtkm_filter)
  vtkm_add_target_information(Histogram
                              DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                              DEVICE_SOURCES Histogram.cxx)
endif()
