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
project(Streamline CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
vtkm_find_gl(OPTIONAL GL GLUT GLEW)

if(TARGET OpenGL::GL AND
   TARGET GLUT::GLUT AND
   TARGET GLEW::GLEW)

  set(gl_libs OpenGL::GL OpenGL::GLU GLEW::GLEW GLUT::GLUT)

  add_executable(StreamLineUniformGrid StreamLineUniformGrid.cxx)
  target_link_libraries(StreamLineUniformGrid PRIVATE vtkm_filter ${gl_libs})

  if(TARGET vtkm::cuda)
    set_source_files_properties(StreamLineUniformGrid.cxx PROPERTIES LANGUAGE "CUDA")
  endif()

endif()
