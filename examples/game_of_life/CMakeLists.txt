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

cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(GameOfLife CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED
  OPTIONAL_COMPONENTS Serial CUDA TBB OpenGL GLUT
  )

if(VTKm_CUDA_FOUND)
  cuda_add_executable(GameOfLife GameOfLife.cu LoadShaders.h)
else()
  add_executable(GameOfLife GameOfLife.cxx LoadShaders.h)
endif()


target_include_directories(GameOfLife PRIVATE ${VTKm_INCLUDE_DIRS})
target_link_libraries(GameOfLife PRIVATE ${VTKm_LIBRARIES})
target_compile_options(GameOfLife PRIVATE ${VTKm_COMPILE_OPTIONS})
