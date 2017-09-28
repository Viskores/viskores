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

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET
  OPTIONAL_COMPONENTS Serial CUDA TBB OpenGL GLUT
  )

if(VTKm_OpenGL_FOUND AND VTKm_GLUT_FOUND)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  add_executable(HelloWorld_SERIAL HelloWorld.cxx LoadShaders.h)
  set_source_files_properties(LoadShaders.h PROPERTIES HEADER_FILE_ONLY TRUE)
  target_include_directories(HelloWorld_SERIAL PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(HelloWorld_SERIAL PRIVATE ${VTKm_LIBRARIES})
  target_compile_options(HelloWorld_SERIAL PRIVATE ${VTKm_COMPILE_OPTIONS})

  if(VTKm_CUDA_FOUND)
    # Cuda compiles do not respect target_include_directories
    cuda_include_directories(${VTKm_INCLUDE_DIRS})
    vtkm_disable_troublesome_thrust_warnings()
    cuda_add_executable(HelloWorld_CUDA HelloWorld.cu LoadShaders.h)
    target_include_directories(HelloWorld_CUDA PRIVATE ${VTKm_INCLUDE_DIRS})
    target_link_libraries(HelloWorld_CUDA PRIVATE ${VTKm_LIBRARIES})
    target_compile_options(HelloWorld_CUDA PRIVATE ${VTKm_COMPILE_OPTIONS})
  endif()

  if(VTKm_TBB_FOUND)
    add_executable(HelloWorld_TBB HelloWorldTBB.cxx LoadShaders.h)
    target_include_directories(HelloWorld_TBB PRIVATE ${VTKm_INCLUDE_DIRS})
    target_link_libraries(HelloWorld_TBB PRIVATE ${VTKm_LIBRARIES})
    target_compile_options(HelloWorld_TBB PRIVATE ${VTKm_COMPILE_OPTIONS})
  endif()

endif()
