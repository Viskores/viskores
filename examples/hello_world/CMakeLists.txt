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

if(OPENGL_FOUND AND GLUT_FOUND)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  add_executable(HelloWorld_SERIAL HelloWorld.cxx)
  target_include_directories(HelloWorld_SERIAL PRIVATE ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${GLUT_INCLUDE_DIR})
  target_link_libraries(HelloWorld_SERIAL ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${GLUT_LIBRARIES})

  if(VTKm_Cuda_FOUND)
    vtkm_disable_troublesome_thrust_warnings()
    cuda_add_executable(HelloWorld_CUDA HelloWorld.cu)
    target_link_libraries(HelloWorld_CUDA ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${GLUT_LIBRARIES})
  endif()

  if(VTKm_TBB_FOUND)
    add_executable(HelloWorld_TBB HelloWorld.cxx)
    target_include_directories(HelloWorld_TBB PRIVATE ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${GLUT_INCLUDE_DIR})
    target_link_libraries(HelloWorld_TBB ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${GLUT_LIBRARIES} ${TBB_LIBRARIES})
  endif()

endif()
