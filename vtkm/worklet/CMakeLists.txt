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
  AverageByKey.h
  CellAverage.h
  CellDeepCopy.h
  CellMeasure.h
  Clip.h
  ContourTreeUniform.h
  ContourTreeUniformAugmented.h
  CoordinateSystemTransform.h
  CosmoTools.h
  CrossProduct.h
  DispatcherMapField.h
  DispatcherMapTopology.h
  DispatcherPointNeighborhood.h
  DispatcherReduceByKey.h
  DispatcherStreamingMapField.h
  DotProduct.h
  ExternalFaces.h
  ExtractGeometry.h
  ExtractPoints.h
  ExtractStructured.h
  FieldEntropy.h
  FieldHistogram.h
  FieldStatistics.h
  Gradient.h
  KdTree3D.h
  KernelSplatter.h
  Keys.h
  LagrangianStructures.h
  Magnitude.h
  Contour.h
  Mask.h
  MaskIndices.h
  MaskNone.h
  MaskPoints.h
  MaskSelect.h
  MeshQuality.h
  NDimsEntropy.h
  NDimsHistMarginalization.h
  NDimsHistogram.h
  Normalize.h
  OrientCellNormals.h
  OrientNormals.h
  OrientPointNormals.h
  OrientPointAndCellNormals.h
  OscillatorSource.h
  ParticleAdvection.h
  PointAverage.h
  PointElevation.h
  PointMerge.h
  PointTransform.h
  Probe.h
  RemoveDegenerateCells.h
  RemoveUnusedPoints.h
  ScalarsToColors.h
  ScatterCounting.h
  ScatterIdentity.h
  ScatterPermutation.h
  ScatterUniform.h
  SplitSharpEdges.h
  StableSortIndices.h
  StreamLineUniformGrid.h
  StreamSurface.h
  SurfaceNormals.h
  Tetrahedralize.h
  Threshold.h
  ThresholdPoints.h
  TriangleWinding.h
  Triangulate.h
  Tube.h
  VertexClustering.h
  WarpScalar.h
  WarpVector.h
  WaveletCompressor.h
  WorkletMapField.h
  WorkletMapTopology.h
  WorkletPointNeighborhood.h
  WorkletReduceByKey.h
  ZFPCompressor.h
  ZFPDecompress.h
  ZFP1DCompressor.h
  ZFP1DDecompress.h
  ZFP2DCompressor.h
  ZFP2DDecompress.h
  )


set(header_impls
  ScalarsToColors.hxx
  Keys.hxx
  )


# This is a list of sources that does not have code that runs on devices. It only
# needs a standard system C++ compiler.
set(sources_no_device
  )

# This is a list of sources that has code that runs on devices and might need to
# be compiled with a device-specific compiler (like CUDA).
set(sources_device
  KeysUnsignedTypes.cxx
  KeysSignedTypes.cxx
  MaskSelect.cxx
  ScatterCounting.cxx
  )


#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(cellmetrics)
add_subdirectory(clip)
add_subdirectory(colorconversion)
add_subdirectory(connectivities)
add_subdirectory(contour)
add_subdirectory(contourtree)
add_subdirectory(contourtree_augmented)
add_subdirectory(cosmotools)
add_subdirectory(gradient)
add_subdirectory(histogram)
add_subdirectory(lcs)
add_subdirectory(moments)
add_subdirectory(splatkernels)
add_subdirectory(spatialstructure)
add_subdirectory(tetrahedralize)
add_subdirectory(triangulate)
add_subdirectory(wavelets)
add_subdirectory(particleadvection)
add_subdirectory(zfp)

vtkm_library(
  NAME vtkm_worklet
  SOURCES ${sources_no_device}
  TEMPLATE_SOURCES ${header_impls}
  HEADERS ${headers}
  DEVICE_SOURCES ${sources_device}
  USE_VTKM_JOB_POOL
  )
target_link_libraries(vtkm_worklet PUBLIC vtkm_cont)

set_source_files_properties(${sources_device} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

#-----------------------------------------------------------------------------
add_subdirectory(testing)
