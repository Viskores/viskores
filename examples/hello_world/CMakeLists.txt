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
project(HelloWorld CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
vtkm_find_gl(OPTIONAL GL GLUT GLEW)

if(TARGET OpenGL::GL AND
   TARGET GLUT::GLUT AND
   TARGET GLEW::GLEW)

  set(gl_libs OpenGL::GL GLEW::GLEW GLUT::GLUT)

  add_executable(HelloWorld HelloWorld.cxx LoadShaders.h)
  target_link_libraries(HelloWorld PRIVATE vtkm_filter ${gl_libs})
  vtkm_add_target_information(HelloWorld
                              MODIFY_CUDA_FLAGS
                              DEVICE_SOURCES HelloWorld.cxx)

endif()
