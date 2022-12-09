##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(StreamlineMPI CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if (VTKm_ENABLE_MPI AND TARGET vtkm::io AND TARGET vtkm::filter_flow)
  add_executable(StreamlineMPI StreamlineMPI.cxx)
  target_compile_definitions(StreamlineMPI PRIVATE "MPI_ENABLED")
  target_link_libraries(StreamlineMPI PRIVATE vtkm::filter_flow vtkm::io MPI::MPI_CXX)
endif()
