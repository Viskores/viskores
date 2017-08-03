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
  CellAverage.h
  CleanGrid.h
  ClipWithField.h
  ClipWithImplicitFunction.h
  ContourTreeUniform.h
  Entropy.h
  ExternalFaces.h
  ExtractGeometry.h
  ExtractPoints.h
  ExtractStructured.h
  FieldMetadata.h
  FilterCell.h
  FilterDataSet.h
  FilterDataSetWithField.h
  FilterField.h
  FilterTraits.h
  Gradient.h
  Histogram.h
  MarchingCubes.h
  Mask.h
  MaskPoints.h
  PointAverage.h
  PointElevation.h
  PolicyBase.h
  PolicyDefault.h
  ResultBase.h
  ResultDataSet.h
  ResultField.h
  SurfaceNormals.h
  Tetrahedralize.h
  Threshold.h
  ThresholdPoints.h
  Triangulate.h
  VectorMagnitude.h
  VertexClustering.h
)

set(header_template_sources
  CellAverage.hxx
  CleanGrid.hxx
  ClipWithField.hxx
  ClipWithImplicitFunction.hxx
  ContourTreeUniform.hxx
  Entropy.hxx
  ExternalFaces.hxx
  ExtractGeometry.hxx
  ExtractPoints.hxx
  ExtractStructured.hxx
  FilterCell.hxx
  FilterDataSet.hxx
  FilterDataSetWithField.hxx
  FilterField.hxx
  Gradient.hxx
  Histogram.hxx
  MarchingCubes.hxx
  Mask.hxx
  MaskPoints.hxx
  PointAverage.hxx
  PointElevation.hxx
  SurfaceNormals.hxx
  Tetrahedralize.hxx
  Threshold.hxx
  ThresholdPoints.hxx
  Triangulate.hxx
  VectorMagnitude.hxx
  VertexClustering.hxx
)

vtkm_declare_headers(${headers})

vtkm_install_template_sources(${header_template_sources})

add_subdirectory(internal)

#-----------------------------------------------------------------------------
add_subdirectory(testing)

