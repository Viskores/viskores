##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

vtkm_add_instantiations(ContourInstantiations FILTER Contour)
vtkm_add_instantiations(GradientInstantiations FILTER Gradient)

vtkm_add_instantiations(ClipWithFieldInstantiations
  FILTER ClipWithField
  INSTANTIATIONS_FILE ClipWithFieldExternInstantiations.h
  )

vtkm_add_instantiations(ClipWithImplicitFunctionInstantiations
  FILTER ClipWithImplicitFunction
  INSTANTIATIONS_FILE ClipWithImplicitFunctionExternInstantiations.h
  )

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
  FilterParticleAdvection.h
  FilterTemporalParticleAdvection.h
  FilterTraits.h
  MapFieldMergeAverage.h
  MapFieldPermutation.h
  PolicyBase.h
  PolicyDefault.h
  TaskQueue.h
  Threshold.h
  ThresholdPoints.h
  Instantiations.h
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
  FilterParticleAdvection.hxx
  FilterTemporalParticleAdvection.hxx
  PointAverage.hxx
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
  PointAverage.cxx
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
  ImageDifference.h
  ImageMedian.h
  Lagrangian.h
  LagrangianStructures.h
  Mask.h
  MaskPoints.h
  MeshQuality.h
  MIRFilter.h
  NDEntropy.h
  NDHistogram.h
  ParticleDensityBase.h
  ParticleDensityCloudInCell.h
  ParticleDensityNearestGridPoint.h
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
  ClipWithFieldExternInstantiations.h
  ClipWithImplicitFunctionExternInstantiations.h
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
  ImageDifference.hxx
  ImageMedian.hxx
  Lagrangian.hxx
  LagrangianStructures.hxx
  Mask.hxx
  MaskPoints.hxx
  MeshQuality.hxx
  MIRFilter.hxx
  NDEntropy.hxx
  NDHistogram.hxx
  ParticleDensityCloudInCell.hxx
  ParticleDensityNearestGridPoint.hxx
  ParticleAdvection.hxx
  Pathline.hxx
  PathParticle.hxx
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
  ${ClipWithFieldInstantiations}
  ${ClipWithImplicitFunctionInstantiations}
  ExternalFaces.cxx
  VectorMagnitude.cxx
  particleadvection/Messenger.cxx
  particleadvection/ParticleMessenger.cxx
)

set(contour_headers
  Contour.h
  ContourTreeUniformAugmented.h
  ContourTreeUniformDistributed.h
  ContourTreeUniform.h
  Slice.h
  )

set(contour_header_template_sources
  Contour.hxx
  ContourTreeUniformAugmented.hxx
  ContourTreeUniformDistributed.hxx
  ContourTreeUniform.hxx
  Slice.hxx
  )

set(contour_sources_device
  Contour.cxx
  ${ContourInstantiations}
  Slice.cxx
  )

set(gradient_headers
  Gradient.h
  )

set(gradient_header_template_sources
  Gradient.hxx
  )

set(gradient_sources_device
  ${GradientInstantiations}
  )

vtkm_pyexpander_generated_file(ClipWithFieldExternInstantiations.h)
vtkm_pyexpander_generated_file(ClipWithImplicitFunctionExternInstantiations.h)

set(core_headers
  NewFilter.h)
set(core_sources_device
  NewFilter.cxx)

vtkm_library(
  NAME vtkm_filter_core
  HEADERS ${core_headers}
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

set_target_properties(
  vtkm_filter_common
  vtkm_filter_core
  vtkm_filter_extra
  vtkm_filter_contour
  vtkm_filter_gradient
  PROPERTIES
  UNITY_BUILD ON
  UNITY_BUILD_MODE BATCH
  )

target_link_libraries(vtkm_filter_common PUBLIC vtkm_worklet)
target_link_libraries(vtkm_filter_core PUBLIC vtkm_cont)
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
  vtkm_filter_common
  vtkm_filter_core
  )

install(TARGETS vtkm_filter EXPORT ${VTKm_EXPORT_NAME})


add_subdirectory(internal)
add_subdirectory(particleadvection)
add_subdirectory(field_transform)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
