##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION 3.8...3.14 FATAL_ERROR)
project(TauTiming CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if(TARGET vtkm_rendering)

  add_executable(Tau_timing TauTiming.cxx)
  target_link_libraries(Tau_timing vtkm_cont)

  if(TARGET vtkm::cuda)
    set_source_files_properties(TauTiming.cxx PROPERTIES LANGUAGE "CUDA")
  endif()
endif()
