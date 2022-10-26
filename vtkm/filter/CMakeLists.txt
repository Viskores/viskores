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
  AmrArrays.h
  CellAverage.h
  CellMeasures.h
  CellSetConnectivity.h
  CleanGrid.h
  ClipWithField.h
  ClipWithImplicitFunction.h
  ComputeMoments.h
  Contour.h
  ContourTreeUniform.h
  ContourTreeUniformAugmented.h
  ContourTreeUniformDistributed.h
  CoordinateSystemTransform.h
  CreateResult.h
  CrossProduct.h
  DotProduct.h
  Entropy.h
  ExternalFaces.h
  ExtractGeometry.h
  ExtractPoints.h
  ExtractStructured.h
  FieldToColors.h
  FieldMetadata.h
  FilterCell.h
  FilterDataSet.h
  FilterDataSet.hxx
  FilterDataSetWithField.h
  FilterDataSetWithField.hxx
  FilterField.h
  FilterField.hxx
  Filter.h
  Filter.hxx
  FilterTraits.h
  GenerateIds.h
  GhostCellClassify.h
  GhostCellRemove.h
  Gradient.h
  Histogram.h
  ImageConnectivity.h
  ImageDifference.h
  ImageMedian.h
  Instantiations.h
  Lagrangian.h
  LagrangianStructures.h
  Mask.h
  MaskPoints.h
  MeshQuality.h
  MIRFilter.h
  NDEntropy.h
  NDHistogram.h
  ParticleAdvection.h
  ParticleDensityCloudInCell.h
  ParticleDensityNearestGridPoint.h
  Pathline.h
  PathParticle.h
  PointAverage.h
  PointElevation.h
  PointTransform.h
  PolicyBase.h
  PolicyDefault.h
  Probe.h
  Slice.h
  SplitSharpEdges.h
  Streamline.h
  StreamSurface.h
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

set_target_properties(
  vtkm_filter_core
  PROPERTIES
  UNITY_BUILD ON
  UNITY_BUILD_MODE BATCH
)

target_link_libraries(vtkm_filter PUBLIC INTERFACE
  vtkm_filter_core
  )

install(TARGETS vtkm_filter EXPORT ${VTKm_EXPORT_NAME})

add_subdirectory(internal)
