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
project(VTKmDemo CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if(TARGET vtkm::rendering AND TARGET vtkm::filter_contour AND TARGET vtkm::source)
  add_executable(Demo Demo.cxx)
  target_link_libraries(Demo PRIVATE vtkm::rendering vtkm::filter_contour vtkm::source)
endif()
