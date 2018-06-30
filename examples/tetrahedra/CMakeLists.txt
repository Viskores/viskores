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
project(Tetrahedra CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
vtkm_find_gl(OPTIONAL GL GLUT GLEW)

if(TARGET OpenGL::GL AND
   TARGET GLUT::GLUT AND
   TARGET GLEW::GLEW)

  set(gl_libs OpenGL::GL OpenGL::GLU GLEW::GLEW GLUT::GLUT)

  add_executable(TetrahedralizeExplicitGrid_SERIAL TetrahedralizeExplicitGrid.cxx)
  add_executable(TriangulateExplicitGrid_SERIAL TriangulateExplicitGrid.cxx)
  add_executable(TetrahedralizeUniformGrid_SERIAL TetrahedralizeUniformGrid.cxx)
  add_executable(TriangulateUniformGrid_SERIAL TriangulateUniformGrid.cxx)

  target_link_libraries(TetrahedralizeExplicitGrid_SERIAL PRIVATE vtkm_cont ${gl_libs})
  target_link_libraries(TriangulateExplicitGrid_SERIAL PRIVATE vtkm_cont ${gl_libs})
  target_link_libraries(TetrahedralizeUniformGrid_SERIAL PRIVATE vtkm_cont ${gl_libs})
  target_link_libraries(TriangulateUniformGrid_SERIAL PRIVATE vtkm_cont ${gl_libs})

  if(TARGET vtkm::cuda)
    add_executable(TetrahedralizeExplicitGrid_CUDA TetrahedralizeExplicitGrid.cu)
    add_executable(TriangulateExplicitGrid_CUDA TriangulateExplicitGrid.cu)
    add_executable(TetrahedralizeUniformGrid_CUDA TetrahedralizeUniformGrid.cu)
    add_executable(TriangulateUniformGrid_CUDA TriangulateUniformGrid.cu)

    target_link_libraries(TetrahedralizeExplicitGrid_CUDA PRIVATE vtkm_cont ${gl_libs})
    target_link_libraries(TriangulateExplicitGrid_CUDA PRIVATE vtkm_cont ${gl_libs})
    target_link_libraries(TetrahedralizeUniformGrid_CUDA PRIVATE vtkm_cont ${gl_libs})
    target_link_libraries(TriangulateUniformGrid_CUDA PRIVATE vtkm_cont ${gl_libs})
  endif()

  if(TARGET vtkm::tbb)
    add_executable(TetrahedralizeExplicitGrid_TBB TetrahedralizeExplicitGridTBB.cxx)
    add_executable(TriangulateExplicitGrid_TBB TriangulateExplicitGridTBB.cxx)
    add_executable(TetrahedralizeUniformGrid_TBB TetrahedralizeUniformGridTBB.cxx)
    add_executable(TriangulateUniformGrid_TBB TriangulateUniformGridTBB.cxx)

    target_link_libraries(TetrahedralizeExplicitGrid_TBB PRIVATE vtkm_cont ${gl_libs})
    target_link_libraries(TriangulateExplicitGrid_TBB PRIVATE vtkm_cont ${gl_libs})
    target_link_libraries(TetrahedralizeUniformGrid_TBB PRIVATE vtkm_cont ${gl_libs})
    target_link_libraries(TriangulateUniformGrid_TBB PRIVATE vtkm_cont ${gl_libs})
  endif()
endif()
