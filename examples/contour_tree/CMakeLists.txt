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
project(ContourTree CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

add_executable(ContourTreeMesh2D ContourTreeMesh2D.cxx)
target_link_libraries(ContourTreeMesh2D vtkm_filter)

add_executable(ContourTreeMesh3D ContourTreeMesh3D.cxx)
target_link_libraries(ContourTreeMesh3D vtkm_filter)

vtkm_add_target_information(ContourTreeMesh2D ContourTreeMesh3D
                            DROP_UNUSED_SYMBOLS
                            MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES
                              ContourTreeMesh2D.cxx ContourTreeMesh3D.cxx)
