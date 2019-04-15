##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(unit_tests
  UnitTestCellAverageFilter.cxx
  UnitTestCellMeasuresFilter.cxx
  UnitTestCellSetConnectivityFilter.cxx
  UnitTestCleanGrid.cxx
  UnitTestClipWithFieldFilter.cxx
  UnitTestClipWithImplicitFunctionFilter.cxx
  UnitTestContourTreeUniformFilter.cxx
  UnitTestContourTreeUniformAugmentedFilter.cxx
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
  UnitTestGhostCellClassify.cxx
  UnitTestGhostCellRemove.cxx
  UnitTestHistogramFilter.cxx
  UnitTestImageConnectivityFilter.cxx
  UnitTestLagrangianFilter.cxx
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
  UnitTestSplitSharpEdgesFilter.cxx
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
  UnitTestZFP.cxx
 )

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES vtkm_filter
  ALL_BACKENDS
  )
