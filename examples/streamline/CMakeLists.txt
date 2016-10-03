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
##  Copyright 2015 Sandia Corporation.
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET
  OPTIONAL_COMPONENTS Serial CUDA TBB OpenGL GLUT
  )

if(VTKm_OpenGL_FOUND AND VTKm_GLUT_FOUND)
  add_executable(StreamLineUniformGrid_SERIAL StreamLineUniformGrid.cxx)
  target_include_directories(StreamLineUniformGrid_SERIAL PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(StreamLineUniformGrid_SERIAL ${VTKm_LIBRARIES})
  target_compile_options(StreamLineUniformGrid_SERIAL PRIVATE ${VTKm_COMPILE_OPTIONS})

  if(VTKm_CUDA_FOUND)
    # Cuda compiles do not respect target_include_directories
    cuda_include_directories(${VTKm_INCLUDE_DIRS})
    cuda_add_executable(StreamLineUniformGrid_CUDA StreamLineUniformGrid.cu)
    target_include_directories(StreamLineUniformGrid_SERIAL PRIVATE ${VTKm_INCLUDE_DIRS})
    target_link_libraries(StreamLineUniformGrid_CUDA ${VTKm_LIBRARIES})
    target_compile_options(StreamLineUniformGrid_CUDA PRIVATE ${VTKm_COMPILE_OPTIONS})
  endif()

  if(VTKm_TBB_FOUND)
    add_executable(StreamLineUniformGrid_TBB StreamLineUniformGridTBB.cxx)
    target_include_directories(StreamLineUniformGrid_TBB PRIVATE ${VTKm_INCLUDE_DIRS})
    target_link_libraries(StreamLineUniformGrid_TBB ${VTKm_LIBRARIES})
    target_compile_options(StreamLineUniformGrid_TBB PRIVATE ${VTKm_COMPILE_OPTIONS})
  endif()

endif()
