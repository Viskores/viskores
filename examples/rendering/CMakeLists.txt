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
  add_executable(Rendering_SERIAL Rendering.cxx)
  target_include_directories(Rendering_SERIAL PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
  target_link_libraries(Rendering_SERIAL ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES} ${VTKm_LIBRARIES})
  target_compile_options(Rendering_SERIAL PRIVATE ${VTKm_COMPILE_OPTIONS})

 # if(VTKm_CUDA_FOUND)
 #   vtkm_disable_troublesome_thrust_warnings()
 #   cuda_add_executable(Rendering_CUDA Rendering.cu)
 #   target_link_libraries(Rendering_CUDA ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
 #   target_compile_options(Rendering_CUDA PRIVATE ${VTKm_COMPILE_OPTIONS})
 # endif()

 # if(VTKm_ENABLE_TBB)
 #   add_executable(Rendering_TBB IsosurfaceUniformGridTBB.cxx)
 #   target_include_directories(Rendering_TBB PRIVATE ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
 #   target_link_libraries(Rendering_TBB ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
 #   target_compile_options(Rendering_TBB PRIVATE ${VTKm_COMPILE_OPTIONS})
 # endif()

endif()
