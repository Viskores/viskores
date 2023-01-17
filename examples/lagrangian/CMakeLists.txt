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

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if(TARGET vtkm::filter_flow)
  add_executable(Lagrangian lagrangian.cxx ABCfield.h)
  target_link_libraries(Lagrangian PRIVATE vtkm::filter_flow)
endif()
