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
  CellAverage.h
  CellMeasures.h
  CellSetConnectivity.h
  CleanGrid.h
  ClipWithField.h
  ClipWithImplicitFunction.h
  ComputeMoments.h
  Contour.h
  CoordinateSystemTransform.h
  CrossProduct.h
  DotProduct.h
  Entropy.h
  ExternalFaces.h
  ExtractGeometry.h
  ExtractPoints.h
  ExtractStructured.h
  FieldToColors.h
  GenerateIds.h
  GhostCellClassify.h
  GhostCellRemove.h
  Gradient.h
  Histogram.h
  ImageConnectivity.h
  ImageDifference.h
  ImageMedian.h
  Instantiations.h
  Mask.h
  MaskPoints.h
  MeshQuality.h
  NDEntropy.h
  NDHistogram.h
  ParticleDensityCloudInCell.h
  ParticleDensityNearestGridPoint.h
  PointAverage.h
  PointElevation.h
  PointTransform.h
  Slice.h
  SplitSharpEdges.h
  SurfaceNormals.h
  Tetrahedralize.h
  Threshold.h
  ThresholdPoints.h
  Triangulate.h
  Tube.h
  VertexClustering.h
  VectorMagnitude.h
  WarpScalar.h
  WarpVector.h
  ZFPCompressor1D.h
  ZFPCompressor2D.h
  ZFPCompressor3D.h
  ZFPDecompressor1D.h
  ZFPDecompressor2D.h
  ZFPDecompressor3D.h
  )

vtkm_declare_headers(${deprecated_headers})

set(common_headers
  CreateResult.h
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
  )

vtkm_declare_headers(${common_headers})

set(common_header_template_sources
  FilterDataSet.hxx
  FilterDataSetWithField.hxx
  FilterField.hxx
  Filter.hxx
  FilterParticleAdvection.hxx
  FilterTemporalParticleAdvection.hxx
  )

vtkm_declare_headers(${common_header_template_sources})

set(extra_headers
  AmrArrays.h
  ContourTreeUniformAugmented.h
  ContourTreeUniformDistributed.h
  ContourTreeUniform.h
  Lagrangian.h
  LagrangianStructures.h
  MIRFilter.h
  ParticleAdvection.h
  Pathline.h
  PathParticle.h
  Probe.h
  Streamline.h
  StreamSurface.h
  )

set(extra_header_template_sources
  AmrArrays.hxx
  ContourTreeUniformAugmented.hxx
  ContourTreeUniformDistributed.hxx
  ContourTreeUniform.hxx
  Lagrangian.hxx
  LagrangianStructures.hxx
  MIRFilter.hxx
  ParticleAdvection.hxx
  Pathline.hxx
  PathParticle.hxx
  Probe.hxx
  Streamline.hxx
  StreamSurface.hxx
  )

set(extra_sources_device
  particleadvection/Messenger.cxx
  particleadvection/ParticleMessenger.cxx
  )

set(core_headers
  FieldSelection.h
  NewFilter.h
  NewFilterField.h
  MapFieldMergeAverage.h
  MapFieldPermutation.h
  TaskQueue.h
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
  NAME vtkm_filter_extra
  TEMPLATE_SOURCES ${extra_header_template_sources}
  HEADERS ${extra_headers}
  DEVICE_SOURCES ${extra_sources_device}
  USE_VTKM_JOB_POOL
)

set_target_properties(
  vtkm_filter_core
  vtkm_filter_extra
  PROPERTIES
  UNITY_BUILD ON
  UNITY_BUILD_MODE BATCH
)

target_link_libraries(vtkm_filter_core PUBLIC vtkm_cont vtkm_worklet)
target_link_libraries(vtkm_filter_extra PUBLIC vtkm_cont vtkm_worklet)
if (VTKm_ENABLE_MPI)
  target_link_libraries(vtkm_filter_extra PUBLIC MPI::MPI_CXX)
endif()

target_link_libraries(vtkm_filter PUBLIC INTERFACE
  vtkm_filter_extra
  vtkm_filter_core
  )

install(TARGETS vtkm_filter EXPORT ${VTKm_EXPORT_NAME})

add_subdirectory(clean_grid)
add_subdirectory(connected_components)
add_subdirectory(contour)
add_subdirectory(density_estimate)
add_subdirectory(entity_extraction)
add_subdirectory(image_processing)
add_subdirectory(internal)
add_subdirectory(particleadvection)
add_subdirectory(field_conversion)
add_subdirectory(field_transform)
add_subdirectory(geometry_refinement)
add_subdirectory(mesh_info)
add_subdirectory(vector_analysis)
add_subdirectory(zfp)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
