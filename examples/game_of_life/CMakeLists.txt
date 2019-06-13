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
project(GameOfLife CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
vtkm_find_gl(OPTIONAL GL GLUT GLEW)

if(TARGET OpenGL::GL AND
   TARGET GLUT::GLUT AND
   TARGET GLEW::GLEW)

  add_executable(GameOfLife GameOfLife.cxx LoadShaders.h)
  if(TARGET vtkm::cuda)
    set_source_files_properties(GameOfLife.cxx PROPERTIES LANGUAGE "CUDA")
  endif()
  target_link_libraries(GameOfLife PRIVATE vtkm_filter OpenGL::GL GLEW::GLEW GLUT::GLUT)
endif()
