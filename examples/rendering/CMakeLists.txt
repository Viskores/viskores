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

vtkm_configure_component_OpenGL()
vtkm_configure_component_Serial()
#vtkm_configure_component_CUDA()
#vtkm_configure_component_TBB()

if(VTKm_OpenGL_FOUND AND GLUT_FOUND)
  add_executable(Rendering_SERIAL Rendering.cxx)
  target_include_directories(Rendering_SERIAL PRIVATE ${GLUT_INCLUDE_DIR} ${VTKm_INCLUDE_DIRS})
  target_link_libraries(Rendering_SERIAL ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
  target_compile_options(Rendering_SERIAL PRIVATE ${VTKm_COMPILE_OPTIONS})

 # if(VTKm_CUDA_FOUND)
 #   vtkm_disable_troublesome_thrust_warnings()
 #   cuda_add_executable(Rendering_CUDA Rendering.cu)
 #   target_include_directories(Rendering_CUDA PRIVATE ${GLUT_INCLUDE_DIR} ${VTKm_INCLUDE_DIRS})
 #   target_link_libraries(Rendering_CUDA ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
 #   target_compile_options(Rendering_CUDA PRIVATE ${VTKm_COMPILE_OPTIONS})
 # endif()

 # if(VTKm_TBB_FOUND)
 #   add_executable(Rendering_TBB IsosurfaceUniformGridTBB.cxx)
 #   target_include_directories(Rendering_TBB PRIVATE ${GLUT_INCLUDE_DIR} ${VTKm_INCLUDE_DIRS})
 #   target_link_libraries(Rendering_TBB ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
 #   target_compile_options(Rendering_TBB PRIVATE ${VTKm_COMPILE_OPTIONS})
 # endif()

endif()
