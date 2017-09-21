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
  OPTIONAL_COMPONENTS Serial OpenGL Rendering GLUT
  )

if(VTKm_OpenGL_FOUND AND VTKm_Rendering_FOUND AND VTKm_GLUT_FOUND)
  add_executable(Rendering_SERIAL Rendering.cxx)
  target_include_directories(Rendering_SERIAL PRIVATE ${VTKm_INCLUDE_DIRS})
  target_link_libraries(Rendering_SERIAL PRIVATE ${VTKm_LIBRARIES})
  target_compile_options(Rendering_SERIAL PRIVATE ${VTKm_COMPILE_OPTIONS})
endif()
