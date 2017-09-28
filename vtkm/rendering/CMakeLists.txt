##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2016 UT-Battelle, LLC.
##  Copyright 2016 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
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
  ColorLegendAnnotation.h
  ColorTable.h
  ConnectivityProxy.h
  DecodePNG.h
  LineRenderer.h
  MatrixHelpers.h
  Scene.h
  Mapper.h
  MapperRayTracer.h
  MapperVolume.h
  MapperConnectivity.h
  MapperWireframer.h
  TextAnnotation.h
  TextAnnotationBillboard.h
  TextAnnotationScreen.h
  TextRenderer.h
  Texture2D.h
  Triangulator.h
  View.h
  View1D.h
  View2D.h
  View3D.h
  WorldAnnotator.h
  )

set(sources
  Actor.cxx
  AxisAnnotation.cxx
  AxisAnnotation2D.cxx
  AxisAnnotation3D.cxx
  BitmapFont.cxx
  BitmapFontFactory.cxx
  BoundingBoxAnnotation.cxx
  Camera.cxx
  Canvas.cxx
  CanvasRayTracer.cxx
  Color.cxx
  ColorBarAnnotation.cxx
  ColorLegendAnnotation.cxx
  ColorTable.cxx
  ConnectivityProxy.cxx
  DecodePNG.cxx
  LineRenderer.cxx
  Mapper.cxx
  MapperRayTracer.cxx
  MapperVolume.cxx
  MapperConnectivity.cxx
  MapperWireframer.cxx
  Scene.cxx
  TextAnnotation.cxx
  TextAnnotationBillboard.cxx
  TextAnnotationScreen.cxx
  TextRenderer.cxx
  View.cxx
  View1D.cxx
  View2D.cxx
  View3D.cxx
  WorldAnnotator.cxx

  internal/RunTriangulator.cxx
  raytracing/BoundingVolumeHierarchy.cxx
  raytracing/Camera.cxx
  raytracing/ChannelBuffer.cxx
  raytracing/Logger.cxx
  raytracing/RayTracer.cxx
  raytracing/RayOperations.cxx
  raytracing/VolumeRendererStructured.cxx
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
  TextureGL.cxx
  WorldAnnotatorGL.cxx
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
  Mapper.cxx
  MapperWireframer.cxx
  Canvas.cxx
  CanvasRayTracer.cxx
  ConnectivityProxy.cxx
  TextRenderer.cxx
  raytracing/BoundingVolumeHierarchy.cxx
  raytracing/Camera.cxx
  raytracing/ChannelBuffer.cxx
  raytracing/VolumeRendererStructured.cxx
  raytracing/RayOperations.cxx
  raytracing/RayTracer.cxx
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

#  vtkm_configure_component_EGL()
#  if(VTKm_EGL_FOUND)
#    list(APPEND headers ${egl_headers})
#    list(APPEND sources ${egl_sources})
#  endif()
endif()

if(VTKm_ENABLE_OSMESA AND NOT VTKm_OSMesa_FOUND)
  message(SEND_ERROR "OSMesa was requested by not properly configured.
Either make sure the OSMesa headers and library are properly found or set
VTKm_ENABLE_OSMESA to OFF.")
endif()

vtkm_declare_headers(${headers})

vtkm_library(
  SOURCES ${sources}
  WRAP_FOR_CUDA ${device_sources}
  )

# EGL Libs are added here to ensure propper linking when statically compiling.
# This is safe to do even when not using EGL as the values will then be empty.
target_link_libraries(vtkm_rendering
                      PUBLIC vtkm_cont
                      PRIVATE ${VTKm_OPENGL_LIBRARIES}
                              ${VTKm_BACKEND_LIBRARIES}
                              ${EGL_LIBRARIES}
                      )

# EGL Dirs are added here to ensure propper linking when statically compiling.
# This is safe to do even when not using EGL as the values will then be empty.
target_include_directories(vtkm_rendering
                          PRIVATE ${VTKm_OPENGL_INCLUDE_DIRS}
                                  ${VTKm_BACKEND_INCLUDE_DIRS}
                                  ${EGL_INCLUDE_DIRS}
                          )

if(UNIX AND NOT APPLE)
  target_link_libraries(vtkm_rendering PRIVATE rt )
endif()


# Subclasses need rendering library
vtkm_configure_component_Rendering()

add_subdirectory(internal)
add_subdirectory(raytracing)

#-----------------------------------------------------------------------------
add_subdirectory(testing)
