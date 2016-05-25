##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2016 Sandia Corporation.
##  Copyright 2016 UT-Battelle, LLC.
##  Copyright 2016 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  AxisAnnotation.h
  AxisAnnotation2D.h
  AxisAnnotation3D.h
  BoundingBoxAnnotation.h
  Color.h
  ColorBarAnnotation.h
  ColorTable.h
  Plot.h
  RenderSurface.h
  RenderSurfaceRayTracer.h
  Scene.h
  SceneRenderer.h
  SceneRendererRayTracer.h
  SceneRendererVolume.h
  Triangulator.h
  View.h
  Window.h
  WorldAnnotator.h
  )

set(opengl_headers
  RenderSurfaceGL.h
  SceneRendererGL.h
  WorldAnnotatorGL.h
  )

set(glx_headers
  RenderSurfaceGLX.h
  )

set(osmesa_headers
  RenderSurfaceOSMesa.h
  )

#-----------------------------------------------------------------------------
if(OPENGL_FOUND)
  set(headers ${headers} ${opengl_headers})

  # Is there a better check for GLX?
  if (UNIX AND NOT APPLE)
    set(headers ${headers} ${glx_headers})

    find_package(MESA)
    if (OSMESA_FOUND)
      set(headers ${headers} ${glx_headers})
    endif()
  endif()
endif()

vtkm_declare_headers(${headers})

add_subdirectory(internal)
add_subdirectory(raytracing)

include_directories(${OSMESA_INCLUDE_DIR})



#-----------------------------------------------------------------------------
add_subdirectory(testing)
