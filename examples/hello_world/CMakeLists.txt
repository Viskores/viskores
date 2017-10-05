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
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(HelloWorld CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
vtkm_find_gl(OPTIONAL GL GLUT GLEW)

if(TARGET OpenGL::GL AND
   TARGET GLUT::GLUT AND
   TARGET GLEW::GLEW)

  add_executable(HelloWorld_SERIAL HelloWorld.cxx LoadShaders.h)
  target_link_libraries(HelloWorld_SERIAL
                        PRIVATE vtkm OpenGL::GL GLEW::GLEW GLUT::GLUT)

  if(TARGET vtkm::tbb)
    add_executable(HelloWorld_TBB HelloWorldTBB.cxx LoadShaders.h)
    target_link_libraries(HelloWorld_TBB
                          PRIVATE vtkm OpenGL::GL GLEW::GLEW GLUT::GLUT)
  endif()

  if(TARGET vtkm::cuda)
    add_executable(HelloWorld_CUDA HelloWorld.cu LoadShaders.h)
    target_link_libraries(HelloWorld_CUDA
                          PRIVATE vtkm OpenGL::GL GLEW::GLEW GLUT::GLUT)
  endif()
endif()
