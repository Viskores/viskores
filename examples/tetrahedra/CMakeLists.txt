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
project(Tetrahedra CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
vtkm_find_gl(OPTIONAL GL GLUT GLEW)

if(TARGET OpenGL::GL AND
   TARGET GLUT::GLUT AND
   TARGET GLEW::GLEW)

  set(gl_libs OpenGL::GL OpenGL::GLU GLEW::GLEW GLUT::GLUT)

  add_executable(TetrahedralizeExplicitGrid TetrahedralizeExplicitGrid.cxx)
  add_executable(TriangulateExplicitGrid TriangulateExplicitGrid.cxx)
  add_executable(TetrahedralizeUniformGrid TetrahedralizeUniformGrid.cxx)
  add_executable(TriangulateUniformGrid TriangulateUniformGrid.cxx)

  target_link_libraries(TetrahedralizeExplicitGrid PRIVATE vtkm_filter ${gl_libs})
  target_link_libraries(TriangulateExplicitGrid PRIVATE vtkm_filter ${gl_libs})
  target_link_libraries(TetrahedralizeUniformGrid PRIVATE vtkm_filter ${gl_libs})
  target_link_libraries(TriangulateUniformGrid PRIVATE vtkm_filter ${gl_libs})

  if(TARGET vtkm::cuda)
  set_source_files_properties(TetrahedralizeExplicitGrid.cxx
                              TriangulateExplicitGrid.cxx
                              TetrahedralizeUniformGrid.cxx
                              TriangulateUniformGrid.cxx PROPERTIES LANGUAGE "CUDA")
  endif()

endif()
