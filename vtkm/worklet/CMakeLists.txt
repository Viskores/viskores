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
  DispatcherMapField.h
  DispatcherStreamingMapField.h
  WorkletMapField.h
  DispatcherMapTopology.h
  WorkletMapTopology.h

  AverageByKey.h
  CellAverage.h
  Clip.h
  ExternalFaces.h
  FieldHistogram.h
  FieldStatistics.h
  KernelSplatter.h
  Magnitude.h
  MarchingCubes.h
  MarchingCubesDataTables.h
  PointElevation.h
  ScatterCounting.h
  ScatterIdentity.h
  ScatterUniform.h
  StreamLineUniformGrid.h
  TetrahedralizeExplicitGrid.h
  TetrahedralizeUniformGrid.h
  TriangulateExplicitGrid.h
  TriangulateUniformGrid.h
  Threshold.h
  VertexClustering.h
  )

#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(splatkernels)

vtkm_declare_headers(${headers})


#-----------------------------------------------------------------------------
add_subdirectory(testing)
