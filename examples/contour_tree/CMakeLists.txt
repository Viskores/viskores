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
project(ContourTree CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if (TARGET vtkm::filter_scalar_topology)
  add_executable(ContourTreeMesh2D ContourTreeMesh2D.cxx)
  target_link_libraries(ContourTreeMesh2D vtkm::filter_core vtkm::filter_scalar_topology)

  add_executable(ContourTreeMesh3D ContourTreeMesh3D.cxx)
  target_link_libraries(ContourTreeMesh3D vtkm::filter_core vtkm::filter_scalar_topology)
endif()
