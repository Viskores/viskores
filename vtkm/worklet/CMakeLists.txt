##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  AverageByKey.h
  CellAverage.h
  CellDeepCopy.h
  CellMeasure.h
  Clip.h
  ContourTreeUniform.h
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
  Magnitude.h
  MarchingCubes.h
  Mask.h
  MaskPoints.h
  NDimsEntropy.h
  NDimsHistogram.h
  NDimsHistMarginalization.h
  Normalize.h
  OscillatorSource.h
  ParticleAdvection.h
  PointAverage.h
  PointElevation.h
  PointTransform.h
  Probe.h
  RemoveUnusedPoints.h
  ScalarsToColors.h
  ScatterCounting.h
  ScatterIdentity.h
  ScatterPermutation.h
  ScatterUniform.h
  StableSortIndices.h
  StreamLineUniformGrid.h
  SurfaceNormals.h
  Tetrahedralize.h
  Threshold.h
  ThresholdPoints.h
  Triangulate.h
  VertexClustering.h
  WarpScalar.h
  WarpVector.h
  WaveletCompressor.h
  WaveletGenerator.h
  WorkletMapField.h
  WorkletMapTopology.h
  WorkletPointNeighborhood.h
  WorkletReduceByKey.h
  )


set(header_impls
  ScalarsToColors.hxx
  )


#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(contour)
add_subdirectory(contourtree)
add_subdirectory(cosmotools)
add_subdirectory(gradient)
add_subdirectory(splatkernels)
add_subdirectory(spatialstructure)
add_subdirectory(tetrahedralize)
add_subdirectory(triangulate)
add_subdirectory(wavelets)
add_subdirectory(particleadvection)

vtkm_declare_headers(${headers})
vtkm_declare_headers(${header_impls} TESTABLE OFF)


#-----------------------------------------------------------------------------
add_subdirectory(testing)
