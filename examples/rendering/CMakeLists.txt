##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
cmake_minimum_required(VERSION 3.3...3.12 FATAL_ERROR)
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

add_executable(RenderingExample_SERIAL Rendering.cxx)
target_link_libraries(RenderingExample_SERIAL PRIVATE vtkm_rendering GLUT::GLUT)

if(TARGET vtkm::tbb)
  add_executable(RenderingExample_TBB Rendering.cxx)
  target_compile_definitions(Rendering_TBB PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB")
  target_link_libraries(RenderingExample_TBB PRIVATE vtkm_rendering GLUT::GLUT)
endif()
