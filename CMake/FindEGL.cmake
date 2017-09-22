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
# Try to find EGL library and include dir.
# Once done this will define
#
# EGL_FOUND        - true if EGL has been found
# EGL_INCLUDE_DIRS  - where the EGL/egl.h and KHR/khrplatform.h can be found
# EGL_LIBRARY      - link this to use libEGL.so.1
# EGL_opengl_LIBRARY     - link with these two libraries instead of the gl library
# EGL_gldispatch_LIBRARY   for full OpenGL support through EGL
# EGL_LIBRARIES    - all EGL related libraries: EGL, OpenGL, GLdispatch


if(NOT EGL_INCLUDE_DIR)

  # If we have a root defined look there first
  if(EGL_ROOT)
    find_path(EGL_INCLUDE_DIR EGL/egl.h PATHS ${EGL_ROOT}/include
      NO_DEFAULT_PATH
    )
  endif()

  if(NOT EGL_INCLUDE_DIR)
    find_path(EGL_INCLUDE_DIR EGL/egl.h PATHS
      /usr/local/include
      /usr/include
    )
  endif()
endif()

if(NOT EGL_LIBRARY)
  # If we have a root defined look there first
  if(EGL_ROOT)
    find_library(EGL_LIBRARY EGL PATHS ${EGL_ROOT}/lib
      NO_DEFAULT_PATH
    )
  endif()

  if(NOT EGL_LIBRARY)
    find_library(EGL_LIBRARY EGL PATHS
      /usr/local/lib
      /usr/lib
    )
  endif()
endif()

if(NOT EGL_opengl_LIBRARY)
  # If we have a root defined look there first
  if(EGL_ROOT)
    find_library(EGL_opengl_LIBRARY OpenGL PATHS ${EGL_ROOT}/lib
      NO_DEFAULT_PATH
    )
  endif()

  if(NOT EGL_opengl_LIBRARY)
    find_library(EGL_opengl_LIBRARY OpenGL PATHS
      /usr/local/lib
      /usr/lib
    )
  endif()
endif()

if(NOT EGL_gldispatch_LIBRARY)
  # If we have a root defined look there first
  if(EGL_ROOT)
    find_library(EGL_gldispatch_LIBRARY GLdispatch PATHS ${EGL_ROOT}/lib
      NO_DEFAULT_PATH
    )
  endif()

  if(NOT EGL_gldispatch_LIBRARY)
    find_library(EGL_gldispatch_LIBRARY GLdispatch PATHS
      /usr/local/lib
      /usr/lib
    )
  endif()

  # For the NVIDIA 358 drivers there isn't a libGLdispath.so. The
  # proper one gets installed as libGLdispatch.so.0.
  if(NOT EGL_gldispatch_LIBRARY)
    find_library(EGL_gldispatch_LIBRARY libGLdispatch.so.0 PATHS
      /usr/local/lib
      /usr/lib
    )
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EGL
                                  FOUND_VAR EGL_FOUND
                                  REQUIRED_VARS EGL_LIBRARY  EGL_opengl_LIBRARY EGL_gldispatch_LIBRARY EGL_INCLUDE_DIR)

if(EGL_FOUND)
  set(EGL_LIBRARIES ${EGL_LIBRARY} ${EGL_opengl_LIBRARY} ${EGL_gldispatch_LIBRARY})
  set(EGL_INCLUDE_DIRS ${EGL_INCLUDE_DIR})
endif()


mark_as_advanced(EGL_DIR EGL_INCLUDE_DIR EGL_LIBRARY EGL_opengl_LIBRARY EGL_gldispatch_LIBRARY)
