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
project(MultiBackend CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED)
find_package(Threads REQUIRED QUIET)

set(headers
  IOGenerator.h
  MultiDeviceGradient.h
  TaskQueue.h
  )

set(device_srcs
  MultiDeviceGradient.cxx
  )

set(srcs
  IOGenerator.cxx
  MultiBackend.cxx
  )

if(TARGET vtkm::cuda)
  set_source_files_properties(${device_srcs} PROPERTIES LANGUAGE "CUDA")
endif()

add_executable(MultiBackend ${device_srcs} ${srcs} ${headers})
target_link_libraries(MultiBackend PRIVATE vtkm_filter Threads::Threads)
