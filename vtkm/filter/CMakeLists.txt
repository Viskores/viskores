##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
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
  CoordinateSystemTransform.h
  CreateResult.h
  CrossProduct.h
  DotProduct.h
  Entropy.h
  ExternalFaces.h
  ExtractGeometry.h
  ExtractPoints.h
  ExtractStructured.h
  FieldMetadata.h
  FieldSelection.h
  FieldToColors.h
  Filter.h
  FilterCell.h
  FilterDataSet.h
  FilterDataSetWithField.h
  FilterField.h
  FilterTraits.h
  GhostCellClassify.h
  GhostCellRemove.h
  Gradient.h
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
  Pathline.h
  PointAverage.h
  PointElevation.h
  PointTransform.h
  PolicyBase.h
  PolicyDefault.h
  PolicyExtrude.h
  Probe.h
  SplitSharpEdges.h
  Streamline.h
  StreamSurface.h
  SurfaceNormals.h
  Tetrahedralize.h
  Threshold.h
  ThresholdPoints.h
  Triangulate.h
  Tube.h
  VectorMagnitude.h
  VertexClustering.h
  WarpScalar.h
  WarpVector.h
  ZFPCompressor1D.h
  ZFPDecompressor1D.h
  ZFPCompressor2D.h
  ZFPDecompressor2D.h
  ZFPCompressor3D.h
  ZFPDecompressor3D.h
)

set(header_template_sources
  CellAverage.hxx
  CellMeasures.hxx
  CellSetConnectivity.hxx
  CleanGrid.hxx
  ClipWithField.hxx
  ClipWithImplicitFunction.hxx
  ComputeMoments.hxx
  Contour.hxx
  ContourTreeUniform.hxx
  ContourTreeUniformAugmented.hxx
  CoordinateSystemTransform.hxx
  CrossProduct.hxx
  DotProduct.hxx
  Entropy.hxx
  ExternalFaces.hxx
  ExtractGeometry.hxx
  ExtractPoints.hxx
  ExtractStructured.hxx
  FieldToColors.hxx
  Filter.hxx
  FilterDataSet.hxx
  FilterDataSetWithField.hxx
  FilterField.hxx
  GhostCellClassify.hxx
  GhostCellRemove.hxx
  Gradient.hxx
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
  Threshold.hxx
  ThresholdPoints.hxx
  Triangulate.hxx
  Tube.hxx
  VectorMagnitude.hxx
  VertexClustering.hxx
  WarpScalar.hxx
  WarpVector.hxx
  ZFPCompressor1D.hxx
  ZFPDecompressor1D.hxx
  ZFPCompressor2D.hxx
  ZFPDecompressor2D.hxx
  ZFPCompressor3D.hxx
  ZFPDecompressor3D.hxx
)
set(sources_device
  CellAverage.cxx
  CleanGrid.cxx
  ClipWithField.cxx
  ClipWithImplicitFunction.cxx
  Contour.cxx
  ExternalFaces.cxx
  ExtractStructured.cxx
  PointAverage.cxx
  Threshold.cxx
  VectorMagnitude.cxx
)

set_source_files_properties(Contour.cxx ExtractStructured.cxx ClipWithField.cxx ClipWithImplicitFunction.cxx PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

vtkm_library(
  NAME vtkm_filter
  TEMPLATE_SOURCES ${header_template_sources}
  HEADERS ${headers}
  DEVICE_SOURCES ${sources_device}
  USE_VTKM_JOB_POOL
  )
target_link_libraries(vtkm_filter PUBLIC vtkm_worklet)

add_subdirectory(internal)

#-----------------------------------------------------------------------------
add_subdirectory(testing)
