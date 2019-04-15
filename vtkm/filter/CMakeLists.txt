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
  ContourTreeUniform.h
  ContourTreeUniformAugmented.h
  CoordinateSystemTransform.h
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
  Lagrangian.h
  MarchingCubes.h
  Mask.h
  MaskPoints.h
  NDEntropy.h
  NDHistogram.h
  OscillatorSource.h
  Pathline.h
  PointAverage.h
  PointElevation.h
  PointTransform.h
  PolicyBase.h
  PolicyDefault.h
  Probe.h
  SplitSharpEdges.h
  Streamline.h
  SurfaceNormals.h
  Tetrahedralize.h
  Threshold.h
  ThresholdPoints.h
  Triangulate.h
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
  FilterCell.hxx
  FilterDataSet.hxx
  FilterDataSetWithField.hxx
  FilterField.hxx
  GhostCellClassify.hxx
  GhostCellRemove.hxx
  Gradient.hxx
  Histogram.hxx
  ImageConnectivity.hxx
  Lagrangian.hxx
  MarchingCubes.hxx
  Mask.hxx
  MaskPoints.hxx
  NDEntropy.hxx
  NDHistogram.hxx
  OscillatorSource.hxx
  Pathline.hxx
  PointAverage.hxx
  PointElevation.hxx
  PointTransform.hxx
  Probe.hxx
  SplitSharpEdges.hxx
  Streamline.hxx
  SurfaceNormals.hxx
  Tetrahedralize.hxx
  Threshold.hxx
  ThresholdPoints.hxx
  Triangulate.hxx
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

vtkm_declare_headers(${headers})
vtkm_declare_headers(${header_template_sources})

# Create an interface library for vtkm_filter. At some point, this will be replaced with a real
# library that contains pre-built filters. That would be created with the vtkm_library CMake
# function (defined in VTKmWrappers.cmake).
add_library(vtkm_filter INTERFACE)
target_link_libraries(vtkm_filter INTERFACE vtkm_worklet)
install(TARGETS vtkm_filter EXPORT ${VTKm_EXPORT_NAME})

add_subdirectory(internal)

#-----------------------------------------------------------------------------
add_subdirectory(testing)
