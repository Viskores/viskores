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
  vtkm_add_target_information(Tau_timing
                              MODIFY_CUDA_FLAGS
                              DEVICE_SOURCES TauTiming.cxx)
  target_link_libraries(Tau_timing vtkm_cont)
endif()
