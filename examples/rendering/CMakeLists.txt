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
  OPTIONAL_COMPONENTS Serial OpenGL Rendering
  )

find_package(GLUT)

if(VTKm_OpenGL_FOUND AND VTKm_Rendering_FOUND AND GLUT_FOUND)
  add_executable(Rendering_SERIAL Rendering.cxx)
  target_include_directories(Rendering_SERIAL PRIVATE ${GLUT_INCLUDE_DIR} ${VTKm_INCLUDE_DIRS})
  target_link_libraries(Rendering_SERIAL ${GLUT_LIBRARIES} ${VTKm_LIBRARIES})
  target_compile_options(Rendering_SERIAL PRIVATE ${VTKm_COMPILE_OPTIONS})
endif()
