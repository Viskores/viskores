##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#unconditionally include rendering contexts so that the helper functions
#are around even if rendering isn't enabled
include(VTKmRenderingContexts)

if(NOT VTKm_ENABLE_RENDERING)
  return()
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
  Cylinderizer.h
  DecodePNG.h # deprecated
  EncodePNG.h # deprecated
  GlyphType.h
  LineRenderer.h
  LineRendererBatcher.h
  MatrixHelpers.h
  Scene.h
  Mapper.h
  MapperCylinder.h
  MapperGlyphBase.h
  MapperGlyphScalar.h
  MapperGlyphVector.h
  MapperPoint.h
  MapperQuad.h
  MapperRayTracer.h
  MapperVolume.h
  MapperConnectivity.h
  MapperWireframer.h
  Quadralizer.h
  ScalarRenderer.h
  TextAnnotation.h
  TextAnnotationBillboard.h
  TextAnnotationScreen.h
  TextRenderer.h
  TextRendererBatcher.h
  Texture2D.h
  Triangulator.h
  View.h
  View1D.h
  View2D.h
  View3D.h
  Wireframer.h
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
  LineRenderer.cxx
  Mapper.cxx
  MapperConnectivity.cxx
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

  raytracing/Logger.cxx
  raytracing/MeshConnectivityContainers.cxx
  raytracing/TriangleExtractor.cxx
  )

# This list of sources has code that uses devices and so might need to be
# compiled with a device-specific compiler (like CUDA).
set(device_sources
  Canvas.cxx
  CanvasRayTracer.cxx
  ConnectivityProxy.cxx
  LineRendererBatcher.cxx
  MapperCylinder.cxx
  MapperGlyphBase.cxx
  MapperGlyphScalar.cxx
  MapperGlyphVector.cxx
  MapperPoint.cxx
  MapperQuad.cxx
  MapperRayTracer.cxx
  MapperVolume.cxx
  MapperWireframer.cxx
  ScalarRenderer.cxx
  TextRendererBatcher.cxx

  internal/RunTriangulator.cxx

  raytracing/BoundingVolumeHierarchy.cxx
  raytracing/Camera.cxx
  raytracing/ChannelBuffer.cxx
  raytracing/ConnectivityTracer.cxx
  raytracing/CylinderExtractor.cxx
  raytracing/CylinderIntersector.cxx
  raytracing/GlyphExtractor.cxx
  raytracing/GlyphExtractorVector.cxx
  raytracing/GlyphIntersector.cxx
  raytracing/GlyphIntersectorVector.cxx
  raytracing/MeshConnectivityBuilder.cxx
  raytracing/QuadExtractor.cxx
  raytracing/QuadIntersector.cxx
  raytracing/RayOperations.cxx
  raytracing/RayTracer.cxx
  raytracing/ScalarRenderer.cxx
  raytracing/ShapeIntersector.cxx
  raytracing/SphereExtractor.cxx
  raytracing/SphereIntersector.cxx
  raytracing/TriangleIntersector.cxx
  raytracing/VolumeRendererStructured.cxx
  )

# the None backend supports not building the opengl version

#-----------------------------------------------------------------------------
vtkm_library(
  NAME vtkm_rendering
  SOURCES ${sources}
  HEADERS ${headers}
  DEVICE_SOURCES ${device_sources}
  )

#-----------------------------------------------------------------------------
target_link_libraries(vtkm_rendering PUBLIC vtkm_filter_image_processing vtkm_filter_entity_extraction vtkm_io)
if(UNIX AND NOT APPLE)
  target_link_libraries(vtkm_rendering PRIVATE rt)
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
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
