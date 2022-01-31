##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(deprecated_headers
  CellSetConnectivity.h
  CleanGrid.h
  ClipWithField.h
  ClipWithImplicitFunction.h
  Contour.h
  CrossProduct.h
  DotProduct.h
  Entropy.h
  ExternalFaces.h
  ExtractGeometry.h
  ExtractPoints.h
  ExtractStructured.h
  GenerateIds.h
  GhostCellRemove.h
  Gradient.h
  Histogram.h
  ImageConnectivity.h
  Mask.h
  MaskPoints.h
  NDEntropy.h
  NDHistogram.h
  ParticleDensityCloudInCell.h
  ParticleDensityNearestGridPoint.h
  Slice.h
  SurfaceNormal.h
  Threshold.h
  ThresholdPoints.h
  VectorMagnitude.h
  )

vtkm_declare_headers(${deprecated_headers})

set(common_headers
  CellAverage.h
  CellMeasures.h
  FieldMetadata.h
  FilterCell.h
  FilterDataSet.h
  FilterDataSetWithField.h
  FilterField.h
  Filter.h
  FilterParticleAdvection.h
  FilterTemporalParticleAdvection.h
  FilterTraits.h
  PolicyBase.h
  PolicyDefault.h
  TaskQueue.h
  Instantiations.h
  )

set(common_header_template_sources
  CellAverage.hxx
  CellMeasures.hxx
  FilterDataSet.hxx
  FilterDataSetWithField.hxx
  FilterField.hxx
  Filter.hxx
  FilterParticleAdvection.hxx
  FilterTemporalParticleAdvection.hxx
  PointAverage.hxx
  )

set(common_sources_device
  CellAverage.cxx
  PointAverage.cxx
)

set(extra_headers
  AmrArrays.h
  ComputeMoments.h
  ContourTreeUniformAugmented.h
  ContourTreeUniformDistributed.h
  ContourTreeUniform.h
  CoordinateSystemTransform.h
  CreateResult.h
  FieldSelection.h
  FieldToColors.h
  GhostCellClassify.h
  ImageDifference.h
  ImageMedian.h
  Lagrangian.h
  LagrangianStructures.h
  MeshQuality.h
  MIRFilter.h
  ParticleAdvection.h
  Pathline.h
  PathParticle.h
  PointAverage.h
  PointElevation.h
  PointTransform.h
  Probe.h
  SplitSharpEdges.h
  Streamline.h
  StreamSurface.h
  Tetrahedralize.h
  Triangulate.h
  Tube.h
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
  AmrArrays.hxx
  ComputeMoments.hxx
  ContourTreeUniformAugmented.hxx
  ContourTreeUniformDistributed.hxx
  ContourTreeUniform.hxx
  CoordinateSystemTransform.hxx
  FieldToColors.hxx
  GhostCellClassify.hxx
  ImageDifference.hxx
  ImageMedian.hxx
  Lagrangian.hxx
  LagrangianStructures.hxx
  MeshQuality.hxx
  MIRFilter.hxx
  ParticleAdvection.hxx
  Pathline.hxx
  PathParticle.hxx
  PointElevation.hxx
  PointTransform.hxx
  Probe.hxx
  SplitSharpEdges.hxx
  Streamline.hxx
  StreamSurface.hxx
  Tetrahedralize.hxx
  Triangulate.hxx
  Tube.hxx
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
  particleadvection/Messenger.cxx
  particleadvection/ParticleMessenger.cxx
  )

set(core_headers
  NewFilter.h
  NewFilterField.h
  MapFieldMergeAverage.h
  MapFieldPermutation.h
  )
set(core_sources
  NewFilterField.cxx
  )
set(core_sources_device
  MapFieldMergeAverage.cxx
  MapFieldPermutation.cxx
  NewFilter.cxx
  )

vtkm_library(
  NAME vtkm_filter_core
  HEADERS ${core_headers}
  SOURCES ${core_sources}
  DEVICE_SOURCES ${core_sources_device}
  USE_VTKM_JOB_POOL
)

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

set_target_properties(
  vtkm_filter_common
  vtkm_filter_core
  vtkm_filter_extra
  PROPERTIES
  UNITY_BUILD ON
  UNITY_BUILD_MODE BATCH
)

target_link_libraries(vtkm_filter_common PUBLIC vtkm_filter_core vtkm_worklet) # TODO: deprecate vtkm_filter_common
target_link_libraries(vtkm_filter_core PUBLIC vtkm_cont vtkm_worklet)
target_link_libraries(vtkm_filter_extra PUBLIC vtkm_filter_common)
if (VTKm_ENABLE_MPI)
  target_link_libraries(vtkm_filter_common PUBLIC MPI::MPI_CXX)
  target_link_libraries(vtkm_filter_extra PUBLIC MPI::MPI_CXX)
endif()

target_link_libraries(vtkm_filter PUBLIC INTERFACE
  vtkm_filter_extra
  vtkm_filter_common
  vtkm_filter_core
  )

install(TARGETS vtkm_filter EXPORT ${VTKm_EXPORT_NAME})

add_subdirectory(clean_grid)
add_subdirectory(connected_components)
add_subdirectory(contour)
add_subdirectory(density_estimate)
add_subdirectory(entity_extraction)
add_subdirectory(internal)
add_subdirectory(particleadvection)
add_subdirectory(field_transform)
add_subdirectory(vector_analysis)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
