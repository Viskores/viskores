##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#-----------------------------------------------------------------------------
# find OpenGL and supporting libraries provided to make using VTK-m easier
# for consumers.
#
# vtkm_find_gl(
#   REQUIRED [GL|GLUT|GLEW]
#   OPTIONAL [GL|GLUT|GLEW]
#   QUIET [ON/OFF]
#   )
#
# Imports everything as imported modules with the following names:
#  - GLUT::GLUT
#  - GLEW::GLEW
#  - OpenGL::GL
# For OpenGL Will also provide the more explicit targets of:
#  - OpenGL::OpenGL
#  - OpenGL::GLU
#  - OpenGL::GLX
#  - OpenGL::EGL
function(vtkm_find_gl)
  set(oneValueArgs QUIET)
  set(multiValueArgs REQUIRED OPTIONAL)
  cmake_parse_arguments(find_gl
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  set(QUIETLY )
  if(find_gl_QUIET)
    set(QUIETLY "QUIET")
  endif()

  foreach(item ${find_gl_REQUIRED})
    set(${item}_REQUIRED "REQUIRED")
    set(DO_${item}_FIND TRUE)
  endforeach()

  foreach(item ${find_gl_OPTIONAL})
    set(DO_${item}_FIND TRUE)
  endforeach()

  #Find GL
  if(DO_GL_FIND AND NOT TARGET OpenGL::GL)
    if(CMAKE_VERSION VERSION_LESS 3.10)
      find_package(OpenGL ${GL_REQUIRED} ${QUIETLY} MODULE)
    else()
      #clunky but we need to make sure we use the upstream module if it exists
      set(orig_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
      set(CMAKE_MODULE_PATH "")
      find_package(OpenGL ${GL_REQUIRED} ${QUIETLY} MODULE)
      set(CMAKE_MODULE_PATH ${orig_CMAKE_MODULE_PATH})
    endif()
  endif()

  #Find GLEW
  if(DO_GLEW_FIND AND NOT TARGET GLEW::GLEW)
    find_package(GLEW ${GLEW_REQUIRED} ${QUIETLY})
  endif()

  if(DO_GLUT_FIND AND NOT TARGET GLUT::GLUT)
    find_package(GLUT ${GLUT_REQUIRED} ${QUIETLY})
  endif()

endfunction()

#-----------------------------------------------------------------------------
if(VTKm_ENABLE_GL_CONTEXT OR
   VTKm_ENABLE_OSMESA_CONTEXT OR
   VTKm_ENABLE_EGL_CONTEXT
   )
  vtkm_find_gl(REQUIRED GL GLEW
               OPTIONAL
               QUIET)
endif()

#-----------------------------------------------------------------------------
if(VTKm_ENABLE_RENDERING AND NOT TARGET vtkm_rendering_gl_context)
  add_library(vtkm_rendering_gl_context INTERFACE)
endif()

#-----------------------------------------------------------------------------
if(VTKm_ENABLE_GL_CONTEXT)
  if(TARGET OpenGL::GLX)
    target_link_libraries(vtkm_rendering_gl_context
                          INTERFACE OpenGL::OpenGL OpenGL::GLX GLEW::GLEW)
  elseif(TARGET OpenGL::GL)
    target_link_libraries(vtkm_rendering_gl_context
                          INTERFACE OpenGL::GL OpenGL::GLU GLEW::GLEW)
  endif()
elseif(VTKm_ENABLE_OSMESA_CONTEXT)
  target_link_libraries(vtkm_rendering_gl_context
                        INTERFACE OpenGL::GL OpenGL::GLU GLEW::GLEW)
elseif(VTKm_ENABLE_EGL_CONTEXT)
  target_link_libraries(vtkm_rendering_gl_context
                        INTERFACE OpenGL::OpenGL OpenGL::EGL GLEW::GLEW)
endif()
