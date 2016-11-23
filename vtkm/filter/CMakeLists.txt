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
  Clip.h
  ExternalFaces.h
  FieldMetadata.h
  FilterCell.h
  FilterDataSet.h
  FilterDataSetWithField.h
  FilterField.h
  FilterTraits.h
  Histogram.h
  MarchingCubes.h
  PointElevation.h
  PolicyBase.h
  PolicyDefault.h
  ResultBase.h
  ResultDataSet.h
  ResultField.h
  Threshold.h
  VertexClustering.h
)

set(header_template_sources
  CellAverage.hxx
  CleanGrid.hxx
  Clip.hxx
  ExternalFaces.hxx
  FilterCell.hxx
  FilterDataSet.hxx
  FilterDataSetWithField.hxx
  FilterField.hxx
  Histogram.hxx
  MarchingCubes.hxx
  PointElevation.hxx
  Threshold.hxx
  VertexClustering.hxx
)

vtkm_declare_headers(${headers})

vtkm_install_template_sources(${header_template_sources})

add_subdirectory(internal)

#-----------------------------------------------------------------------------
add_subdirectory(testing)

