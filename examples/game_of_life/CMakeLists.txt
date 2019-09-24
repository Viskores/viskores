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
project(GameOfLife CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
vtkm_find_gl(OPTIONAL GL GLUT GLEW)

if(TARGET OpenGL::GL AND
   TARGET GLUT::GLUT AND
   TARGET GLEW::GLEW)

  add_executable(GameOfLife GameOfLife.cxx LoadShaders.h)
  target_link_libraries(GameOfLife PRIVATE vtkm_filter OpenGL::GL GLEW::GLEW GLUT::GLUT)
  vtkm_add_target_information(GameOfLife
                              DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                              DEVICE_SOURCES GameOfLife.cxx)
endif()
