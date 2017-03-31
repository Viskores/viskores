##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 Sandia Corporation.
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
  Clip.h
  ContourTreeUniform.h
  DispatcherMapField.h
  DispatcherMapTopology.h
  DispatcherReduceByKey.h
  DispatcherStreamingMapField.h
  ExternalFaces.h
  ExtractGeometry.h
  ExtractPoints.h
  FieldHistogram.h
  FieldStatistics.h
  Gradient.h
  KernelSplatter.h
  Keys.h
  Magnitude.h
  MarchingCubes.h
  MarchingCubesDataTables.h
  Mask.h
  MaskPoints.h
  PointAverage.h
  PointElevation.h
  RemoveUnusedPoints.h
  ScatterCounting.h
  ScatterIdentity.h
  ScatterUniform.h
  StreamLineUniformGrid.h
  Tetrahedralize.h
  Threshold.h
  ThresholdPoints.h
  Triangulate.h
  VertexClustering.h
  WaveletCompressor.h
  WorkletMapField.h
  WorkletMapTopology.h
  WorkletReduceByKey.h
  )

#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(contourtree)
add_subdirectory(splatkernels)
add_subdirectory(tetrahedralize)
add_subdirectory(triangulate)
add_subdirectory(wavelets)

vtkm_declare_headers(${headers})


#-----------------------------------------------------------------------------
add_subdirectory(testing)
