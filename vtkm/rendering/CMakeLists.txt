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

#unconditionally include rendering contexts so that the helper functions
#are around even if rendering isn't enabled
include(VTKmRenderingContexts)

if(NOT VTKm_ENABLE_RENDERING)
  return()
endif()

# determine what context(s) for rendering we want to build
vtkm_option(VTKm_ENABLE_GL_CONTEXT "Enable GL context for vtkm rendering" OFF)
if(UNIX AND NOT APPLE)
  vtkm_option(VTKm_ENABLE_OSMESA_CONTEXT "Enable OSMesa context for vtkm rendering" OFF)
  vtkm_option(VTKm_ENABLE_EGL_CONTEXT "Enable EGL context for vtkm rendering" OFF)
  mark_as_advanced(VTKm_ENABLE_OSMESA_CONTEXT)
  mark_as_advanced(VTKm_ENABLE_EGL_CONTEXT)
endif()
if(VTKm_ENABLE_GL_CONTEXT AND VTKm_ENABLE_OSMESA_CONTEXT)
  message(FATAL_ERROR "VTK-m GL and OSMesa contexts are mutually exclusive")
endif()
if(VTKm_ENABLE_GL_CONTEXT AND VTKm_ENABLE_EGL_CONTEXT)
  message(FATAL_ERROR "VTK-m GL and EGL contexts are mutually exclusive")
endif()
if(VTKm_ENABLE_EGL_CONTEXT AND VTKm_ENABLE_OSMESA_CONTEXT)
  message(FATAL_ERROR "VTK-m EGL and OSMesa contexts are mutually exclusive")
endif()

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
  Color.cxx
  ColorBarAnnotation.cxx
  ColorLegendAnnotation.cxx
  DecodePNG.cxx
  LineRenderer.cxx
  MapperConnectivity.cxx
  MapperRayTracer.cxx
  MapperVolume.cxx
  Scene.cxx
  TextAnnotation.cxx
  TextAnnotationBillboard.cxx
  TextAnnotationScreen.cxx
  View.cxx
  View1D.cxx
  View2D.cxx
  View3D.cxx
  WorldAnnotator.cxx

  raytracing/ConnectivityBase.cxx
  raytracing/ConnectivityTracerBase.cxx
  raytracing/ConnectivityTracerFactory.cxx
  raytracing/Logger.cxx
  )

# Note that EGL and OSMesa Canvas depend on the GL version being built. Only
# the None backend supports not building the opengl version
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
  Canvas.cxx
  CanvasRayTracer.cxx
  ConnectivityProxy.cxx
  Mapper.cxx
  MapperWireframer.cxx
  TextRenderer.cxx

  internal/RunTriangulator.cxx
  raytracing/BoundingVolumeHierarchy.cxx
  raytracing/Camera.cxx
  raytracing/ChannelBuffer.cxx
  raytracing/ConnectivityTracer.cxx
  raytracing/RayOperations.cxx
  raytracing/RayTracer.cxx
  raytracing/VolumeRendererStructured.cxx
  )

#-----------------------------------------------------------------------------
vtkm_library(
  NAME vtkm_rendering
  SOURCES ${sources}
  HEADERS ${headers}
  WRAP_FOR_CUDA ${device_sources}
  )

if(VTKm_ENABLE_GL_CONTEXT)
  vtkm_declare_headers(${opengl_headers})
  target_sources(vtkm_rendering PRIVATE ${opengl_sources})
elseif(VTKm_ENABLE_OSMESA_CONTEXT)
  vtkm_declare_headers(${opengl_headers} ${osmesa_headers})
  target_sources(vtkm_rendering PRIVATE ${opengl_sources} ${osmesa_sources})
elseif(VTKm_ENABLE_EGL_CONTEXT)
  vtkm_declare_headers(${opengl_headers} ${egl_headers})
  target_sources(vtkm_rendering PRIVATE ${opengl_sources} ${egl_sources})
endif()

#-----------------------------------------------------------------------------
target_link_libraries(vtkm_rendering PUBLIC vtkm_cont)


if(UNIX AND NOT APPLE)
  target_link_libraries(vtkm_rendering PRIVATE rt )
endif()

#-----------------------------------------------------------------------------
target_link_libraries(vtkm_rendering PUBLIC vtkm_rendering_gl_context)

if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
  install(TARGETS vtkm_rendering_gl_context
    EXPORT ${VTKm_EXPORT_NAME}
    )
endif()

#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(raytracing)
add_subdirectory(testing)
