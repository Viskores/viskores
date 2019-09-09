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
project(MultiBackend CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED)
find_package(Threads REQUIRED QUIET)

set(headers
  IOGenerator.h
  MultiDeviceGradient.h
  TaskQueue.h
  )

set(srcs
  MultiBackend.cxx
  MultiDeviceGradient.cxx
  IOGenerator.cxx
  )

add_executable(MultiBackend ${srcs} ${headers})
target_link_libraries(MultiBackend PRIVATE vtkm_filter Threads::Threads)
vtkm_add_target_information(MultiBackend
                            DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES ${srcs})
