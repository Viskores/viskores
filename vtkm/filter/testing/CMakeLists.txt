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

set(unit_tests
  UnitTestCellAverageFilter.cxx
  UnitTestCellMeasuresFilter.cxx
  UnitTestCleanGrid.cxx
  UnitTestClipWithFieldFilter.cxx
  UnitTestClipWithImplicitFunctionFilter.cxx
  UnitTestContourTreeUniformFilter.cxx
  UnitTestCoordinateSystemTransform.cxx
  UnitTestCrossProductFilter.cxx
  UnitTestDotProductFilter.cxx
  UnitTestEntropyFilter.cxx
  UnitTestExternalFacesFilter.cxx
  UnitTestExtractGeometryFilter.cxx
  UnitTestExtractPointsFilter.cxx
  UnitTestExtractStructuredFilter.cxx
  UnitTestFieldMetadata.cxx
  UnitTestFieldSelection.cxx
  UnitTestFieldToColors.cxx
  UnitTestGradient.cxx
  UnitTestHistogramFilter.cxx
  UnitTestMarchingCubesFilter.cxx
  UnitTestMaskFilter.cxx
  UnitTestMaskPointsFilter.cxx
  UnitTestMultiBlockFilters.cxx
  UnitTestMultiBlockHistogramFilter.cxx
  UnitTestNDEntropyFilter.cxx
  UnitTestNDHistogramFilter.cxx
  UnitTestPointAverageFilter.cxx
  UnitTestPointElevationFilter.cxx
  UnitTestPointTransform.cxx
  UnitTestProbe.cxx
  UnitTestStreamlineFilter.cxx
  UnitTestSurfaceNormalsFilter.cxx
  UnitTestTetrahedralizeFilter.cxx
  UnitTestThresholdFilter.cxx
  UnitTestThresholdPointsFilter.cxx
  UnitTestTriangulateFilter.cxx
  UnitTestVectorMagnitudeFilter.cxx
  UnitTestVertexClusteringFilter.cxx
  UnitTestWarpScalarFilter.cxx
  UnitTestWarpVectorFilter.cxx
 )

vtkm_unit_tests(SOURCES ${unit_tests})
