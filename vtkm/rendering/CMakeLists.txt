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
  Actor.h
  AxisAnnotation.h
  AxisAnnotation2D.h
  AxisAnnotation3D.h
  BitmapFont.h
  BitmapFontFactory.h
  BoundingBoxAnnotation.h
  Camera.h
  Canvas.h
  CanvasRayTracer.h
  Color.h
  ColorBarAnnotation.h
  ColorTable.h
  MatrixHelpers.h
  Scene.h
  Mapper.h
  MapperRayTracer.h
  MapperVolume.h
  TextAnnotation.h
  Triangulator.h
  View.h
  WorldAnnotator.h
  )

set(sources
  AxisAnnotation2D.cxx
  )

set(opengl_headers
  CanvasGL.h
  MapperGL.h
  TextureGL.h
  WorldAnnotatorGL.h
  )
set(egl_headers
  CanvasEGL.h
)

set(osmesa_headers
  CanvasOSMesa.h
  )

#-----------------------------------------------------------------------------
vtkm_configure_component_OpenGL()
if(VTKm_OpenGL_FOUND)
  set(headers ${headers} ${opengl_headers})
  
  vtkm_configure_component_OSMesa()
  if(VTKm_OSMesa_FOUND)
    set(headers ${headers} ${osmesa_headers})
  endif()

  vtkm_configure_component_EGL()
  if(VTKm_EGL_FOUND)
    set(headers ${headers} ${egl_headers})
  endif()

  vtkm_configure_component_GLFW()
  if(VTKm_GLFW_FOUND)
    set(headers ${headers} ${glfw_headers})
  endif()
endif()

vtkm_declare_headers(${headers})

vtkm_library(SOURCES ${sources})

# Subclasses need rendering library
vtkm_configure_component_Rendering()

add_subdirectory(internal)
add_subdirectory(raytracing)




#-----------------------------------------------------------------------------
add_subdirectory(testing)
