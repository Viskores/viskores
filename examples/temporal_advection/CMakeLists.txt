##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#Find the VTK-m package
cmake_minimum_required(VERSION 3.8...3.14 FATAL_ERROR)
project(TemporalAdvection CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

add_executable(Temporal_Advection TemporalAdvection.cxx)
target_link_libraries(Temporal_Advection PRIVATE vtkm_filter)

if(TARGET vtkm::cuda)
  set_source_files_properties(TemporalAdvection.cxx PROPERTIES LANGUAGE "CUDA")
endif()
