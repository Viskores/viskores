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
project(IsoSurface CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
vtkm_find_gl(OPTIONAL GL GLUT GLEW)

if(TARGET OpenGL::GL AND
   TARGET GLUT::GLUT AND
   TARGET GLEW::GLEW)

  set_source_files_properties(quaternion.h PROPERTIES HEADER_FILE_ONLY TRUE)

  set(gl_libs OpenGL::GL OpenGL::GLU GLEW::GLEW GLUT::GLUT)
  add_executable(IsosurfaceUniformGrid_SERIAL IsosurfaceUniformGrid.cxx quaternion.h)
  target_link_libraries(IsosurfaceUniformGrid_SERIAL PRIVATE vtkm_cont ${gl_libs})

  if(TARGET vtkm::cuda)
    add_executable(IsosurfaceUniformGrid_CUDA IsosurfaceUniformGrid.cu quaternion.h)
    target_link_libraries(IsosurfaceUniformGrid_CUDA PRIVATE vtkm_cont ${gl_libs})
  endif()

  if(TARGET vtkm::tbb)
    add_executable(IsosurfaceUniformGrid_TBB IsosurfaceUniformGridTBB.cxx quaternion.h)
    target_link_libraries(IsosurfaceUniformGrid_TBB PRIVATE vtkm_cont ${gl_libs})
  endif()

endif()
