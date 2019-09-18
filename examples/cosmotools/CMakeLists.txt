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
project(CosmoTools CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

add_executable(CosmoCenterFinder CosmoCenterFinder.cxx)
add_executable(CosmoHaloFinder CosmoHaloFinder.cxx)
target_link_libraries(CosmoCenterFinder PRIVATE vtkm_filter)
target_link_libraries(CosmoHaloFinder PRIVATE vtkm_filter)

vtkm_add_target_information(CosmoCenterFinder CosmoHaloFinder
                            DROP_UNUSED_SYMBOLS
                            MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES
                              CosmoCenterFinder.cxx CosmoHaloFinder.cxx)
