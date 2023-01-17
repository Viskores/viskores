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
project(Tetrahedra CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if(TARGET vtkm::filter_geometry_refinement AND TARGET vtkm::io)
  add_executable(Tetrahedralize Tetrahedralize.cxx)
  target_link_libraries(Tetrahedralize PRIVATE vtkm::filter_geometry_refinement vtkm::io)

  add_executable(Triangulate Triangulate.cxx)
  target_link_libraries(Triangulate PRIVATE vtkm::filter_geometry_refinement vtkm::io)
endif()
