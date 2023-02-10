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
project(HelloWorklet CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if(TARGET vtkm::io)
  add_executable(HelloWorklet HelloWorklet.cxx)
  target_link_libraries(HelloWorklet PRIVATE vtkm::filter_core vtkm::io)

  vtkm_add_target_information(HelloWorklet
    DROP_UNUSED_SYMBOLS
    MODIFY_CUDA_FLAGS
    DEVICE_SOURCES HelloWorklet.cxx)
endif()
