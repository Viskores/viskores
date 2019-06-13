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
project(RenderingExample CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if(NOT TARGET vtkm_rendering OR NOT VTKm_ENABLE_GL_CONTEXT)
  return()
endif()

vtkm_find_gl(OPTIONAL GLUT)
if(NOT TARGET GLUT::GLUT)
  return()
endif()

add_executable(RenderingExample Rendering.cxx)
target_link_libraries(RenderingExample PRIVATE vtkm_rendering GLUT::GLUT)
