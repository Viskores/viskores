##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION 3.12...3.19 FATAL_ERROR)
project(LogisticMap CXX)

find_package(VTKm REQUIRED QUIET)

add_executable(LogisticMap LogisticMap.cxx)
target_link_libraries(LogisticMap PRIVATE vtkm_io)

vtkm_add_target_information(LogisticMap
                            DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES LogisticMap.cxx)
