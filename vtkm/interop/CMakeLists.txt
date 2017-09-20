##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

# Determine if we actually want to compile OpenGL Interop.
# We defer declaration of the option because whether we offer it depends on
# dependent components that are not loaded in the base directories.
vtkm_configure_component_OpenGL()

include(CMakeDependentOption)

cmake_dependent_option(
  VTKm_ENABLE_OPENGL_INTEROP "Enable OpenGL Interop" ON
  "VTKm_OpenGL_FOUND" OFF)

if(VTKm_ENABLE_OPENGL_INTEROP)
  vtkm_configure_component_Interop()
  if(NOT VTKm_Interop_FOUND)
    message(SEND_ERROR "Could not configure for OpenGL Interop. Either configure necessary subcomponents or turn off VTKm_ENABLE_OPENGL_INTEROP.")
  endif()

  set(headers
    BufferState.h
    TransferToOpenGL.h
    )

  #-----------------------------------------------------------------------------
  add_subdirectory(internal)

  #-----------------------------------------------------------------------------
  vtkm_declare_headers(${headers})

  add_subdirectory(testing)
endif()
