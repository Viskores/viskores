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
  DecodePNG.h
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
  Actor.cxx
  AxisAnnotation.cxx
  AxisAnnotation2D.cxx
  AxisAnnotation3D.cxx
  BitmapFont.cxx
  BitmapFontFactory.cxx
  Camera.cxx
  Canvas.cxx
  CanvasRayTracer.cxx
  ColorBarAnnotation.cxx
  ColorTable.cxx
  DecodePNG.cxx
  Mapper.cxx
  MapperRayTracer.cxx
  MapperVolume.cxx
  Scene.cxx

  internal/RunTriangulator.cxx
  )

set(opengl_headers
  CanvasGL.h
  MapperGL.h
  TextureGL.h
  WorldAnnotatorGL.h
  )

set(opengl_sources
  CanvasGL.cxx
  MapperGL.cxx
  )

set(egl_headers
  CanvasEGL.h
)

set(egl_sources
  CanvasEGL.cxx
  )

set(osmesa_headers
  CanvasOSMesa.h
  )

set(osmesa_sources
  CanvasOSMesa.cxx
  )

# This list of sources has code that uses devices and so might need to be
# compiled with a device-specific compiler (like CUDA).
set(device_sources
  CanvasRayTracer.cxx
  )

#-----------------------------------------------------------------------------
vtkm_configure_component_OpenGL()
if(VTKm_OpenGL_FOUND)
  list(APPEND headers ${opengl_headers})
  list(APPEND sources ${opengl_sources})
  
  vtkm_configure_component_OSMesa()
  if(VTKm_OSMesa_FOUND)
    list(APPEND headers ${osmesa_headers})
    list(APPEND sources ${osmesa_sources})
  endif()

  vtkm_configure_component_EGL()
  if(VTKm_EGL_FOUND)
    list(APPEND headers ${egl_headers})
    list(APPEND sources ${egl_headers})
  endif()

  vtkm_configure_component_GLFW()
  if(VTKm_GLFW_FOUND)
    list(APPEND headers ${glfw_headers})
  endif()
endif()

vtkm_declare_headers(${headers})

if (VTKm_ENABLE_CUDA)
  vtkm_library(
    SOURCES ${sources}
    CUDA
    WRAP_FOR_CUDA ${device_sources}
    )
else()
  vtkm_library(
    SOURCES ${sources}
    )
endif()

# Subclasses need rendering library
vtkm_configure_component_Rendering()

add_subdirectory(internal)
add_subdirectory(raytracing)




#-----------------------------------------------------------------------------
add_subdirectory(testing)
