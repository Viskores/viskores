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

  vtkm_add_target_information(TetrahedralizeExplicitGrid
                            MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES TetrahedralizeExplicitGrid.cxx)
  vtkm_add_target_information(TriangulateExplicitGrid
                            MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES TriangulateExplicitGrid.cxx)
  vtkm_add_target_information(TetrahedralizeUniformGrid
                            MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES TetrahedralizeUniformGrid.cxx)
  vtkm_add_target_information(TriangulateUniformGrid
                            MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES TriangulateUniformGrid.cxx)

endif()
