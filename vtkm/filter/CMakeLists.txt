##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(common_headers
  CellAverage.h
  CellMeasures.h
  CleanGrid.h
  ExtractGeometry.h
  ExtractPoints.h
  ExtractStructured.h
  FieldMetadata.h
  FilterCell.h
  FilterDataSet.h
  FilterDataSetWithField.h
  FilterField.h
  Filter.h
  FilterTraits.h
  MapFieldMergeAverage.h
  MapFieldPermutation.h
  PolicyBase.h
  PolicyDefault.h
  PolicyExtrude.h
  Threshold.h
  ThresholdPoints.h
  )

set(common_header_template_sources
  CellAverage.hxx
  CellMeasures.hxx
  CleanGrid.hxx
  ExtractGeometry.hxx
  ExtractPoints.hxx
  ExtractStructured.hxx
  FilterDataSet.hxx
  FilterDataSetWithField.hxx
  FilterField.hxx
  Filter.hxx
  Threshold.hxx
  ThresholdPoints.hxx
  )

set(common_sources_device
  CellAverage.cxx
  CleanGrid.cxx
  ExtractGeometry.cxx
  ExtractStructured.cxx
  MapFieldMergeAverage.cxx
  MapFieldPermutation.cxx
  Threshold.cxx
)

set(extra_headers
  CellSetConnectivity.h
  ClipWithField.h
  ClipWithImplicitFunction.h
  ComputeMoments.h
  CoordinateSystemTransform.h
  CreateResult.h
  CrossProduct.h
  DotProduct.h
  Entropy.h
  ExternalFaces.h
  FieldSelection.h
  FieldToColors.h
  GhostCellClassify.h
  GhostCellRemove.h
  Histogram.h
  ImageConnectivity.h
  ImageMedian.h
  Lagrangian.h
  LagrangianStructures.h
  Mask.h
  MaskPoints.h
  MeshQuality.h
  NDEntropy.h
  NDHistogram.h
  ParticleAdvection.h
  Pathline.h
  PointAverage.h
  PointElevation.h
  PointTransform.h
  Probe.h
  SplitSharpEdges.h
  Streamline.h
  StreamSurface.h
  SurfaceNormals.h
  Tetrahedralize.h
  Triangulate.h
  Tube.h
  VectorMagnitude.h
  VertexClustering.h
  WarpScalar.h
  WarpVector.h
  ZFPCompressor1D.h
  ZFPCompressor2D.h
  ZFPCompressor3D.h
  ZFPDecompressor1D.h
  ZFPDecompressor2D.h
  ZFPDecompressor3D.h
  )

set(extra_header_template_sources
  CellSetConnectivity.hxx
  ClipWithField.hxx
  ClipWithImplicitFunction.hxx
  ComputeMoments.hxx
  CoordinateSystemTransform.hxx
  CrossProduct.hxx
  DotProduct.hxx
  Entropy.hxx
  ExternalFaces.hxx
  FieldToColors.hxx
  GhostCellClassify.hxx
  GhostCellRemove.hxx
  Histogram.hxx
  ImageConnectivity.hxx
  ImageMedian.hxx
  Lagrangian.hxx
  LagrangianStructures.hxx
  Mask.hxx
  MaskPoints.hxx
  MeshQuality.hxx
  NDEntropy.hxx
  NDHistogram.hxx
  ParticleAdvection.hxx
  Pathline.hxx
  PointAverage.hxx
  PointElevation.hxx
  PointTransform.hxx
  Probe.hxx
  SplitSharpEdges.hxx
  Streamline.hxx
  StreamSurface.hxx
  SurfaceNormals.hxx
  Tetrahedralize.hxx
  Triangulate.hxx
  Tube.hxx
  VectorMagnitude.hxx
  VertexClustering.hxx
  WarpScalar.hxx
  WarpVector.hxx
  ZFPCompressor1D.hxx
  ZFPCompressor2D.hxx
  ZFPCompressor3D.hxx
  ZFPDecompressor1D.hxx
  ZFPDecompressor2D.hxx
  ZFPDecompressor3D.hxx
  )

set(extra_sources_device
  ClipWithField.cxx
  ClipWithImplicitFunction.cxx
  PointAverage.cxx
  VectorMagnitude.cxx
  ExternalFaces.cxx
  particleadvection/Messenger.cxx
  particleadvection/ParticleMessenger.cxx
)

set(contour_headers
  Contour.h
  ContourTreeUniformAugmented.h
  ContourTreeUniformDistributed.h
  ContourTreeUniform.h
  )

set(contour_header_template_sources
  Contour.hxx
  ContourTreeUniformAugmented.hxx
  ContourTreeUniformDistributed.hxx
  ContourTreeUniform.hxx
  )

set(contour_sources_device
  Contour.cxx
  ContourInteger.cxx
  ContourScalar.cxx
  )

set(gradient_headers
  Gradient.h
  )

set(gradient_header_template_sources
  Gradient.hxx
  )

set(gradient_sources_device
  GradientScalar.cxx
  GradientUniformPoints.cxx
  GradientVector.cxx
  )

set_source_files_properties(Contour.cxx ExtractStructured.cxx ClipWithField.cxx ClipWithImplicitFunction.cxx PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

add_library(vtkm_filter INTERFACE)

vtkm_library(
  NAME vtkm_filter_common
  TEMPLATE_SOURCES ${common_header_template_sources}
  HEADERS ${common_headers}
  DEVICE_SOURCES ${common_sources_device}
  USE_VTKM_JOB_POOL
  )

vtkm_library(
  NAME vtkm_filter_extra
  TEMPLATE_SOURCES ${extra_header_template_sources}
  HEADERS ${extra_headers}
  DEVICE_SOURCES ${extra_sources_device}
  USE_VTKM_JOB_POOL
  )

vtkm_library(
  NAME vtkm_filter_contour
  TEMPLATE_SOURCES ${contour_header_template_sources}
  HEADERS ${contour_headers}
  DEVICE_SOURCES ${contour_sources_device}
  USE_VTKM_JOB_POOL
  )

vtkm_library(
  NAME vtkm_filter_gradient
  TEMPLATE_SOURCES ${gradient_header_template_sources}
  HEADERS ${gradient_headers}
  DEVICE_SOURCES ${gradient_sources_device}
  USE_VTKM_JOB_POOL
  )

target_link_libraries(vtkm_filter_common PUBLIC vtkm_worklet)
target_link_libraries(vtkm_filter_extra PUBLIC vtkm_filter_common)
target_link_libraries(vtkm_filter_contour PUBLIC vtkm_filter_common)
target_link_libraries(vtkm_filter_gradient PUBLIC vtkm_filter_common)
if (VTKm_ENABLE_MPI)
  target_link_libraries(vtkm_filter_common PUBLIC MPI::MPI_CXX)
  target_link_libraries(vtkm_filter_extra PUBLIC MPI::MPI_CXX)
  target_link_libraries(vtkm_filter_contour PUBLIC MPI::MPI_CXX)
  target_link_libraries(vtkm_filter_gradient PUBLIC MPI::MPI_CXX)
endif()
target_link_libraries(vtkm_filter PUBLIC INTERFACE
  vtkm_filter_extra
  vtkm_filter_contour
  vtkm_filter_gradient
  vtkm_filter_common)

install(TARGETS vtkm_filter EXPORT ${VTKm_EXPORT_NAME})


add_subdirectory(internal)
add_subdirectory(particleadvection)

#-----------------------------------------------------------------------------
add_subdirectory(testing)
