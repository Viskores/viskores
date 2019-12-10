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
  UnitTestContourFilter.cxx
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
  UnitTestImageMedianFilter.cxx
  UnitTestLagrangianFilter.cxx
  UnitTestLagrangianStructuresFilter.cxx
  UnitTestMaskFilter.cxx
  UnitTestMaskPointsFilter.cxx
  UnitTestMeshQualityFilter.cxx
  UnitTestNDEntropyFilter.cxx
  UnitTestNDHistogramFilter.cxx
  UnitTestPartitionedDataSetFilters.cxx
  UnitTestPartitionedDataSetHistogramFilter.cxx
  UnitTestPointAverageFilter.cxx
  UnitTestPointElevationFilter.cxx
  UnitTestPointTransform.cxx
  UnitTestProbe.cxx
  UnitTestSplitSharpEdgesFilter.cxx
  UnitTestStreamlineFilter.cxx
  UnitTestStreamSurfaceFilter.cxx
  UnitTestSurfaceNormalsFilter.cxx
  UnitTestTetrahedralizeFilter.cxx
  UnitTestThresholdFilter.cxx
  UnitTestThresholdPointsFilter.cxx
  UnitTestTriangulateFilter.cxx
  UnitTestTubeFilter.cxx
  UnitTestVectorMagnitudeFilter.cxx
  UnitTestVertexClusteringFilter.cxx
  UnitTestWarpScalarFilter.cxx
  UnitTestWarpVectorFilter.cxx
  UnitTestZFP.cxx
 )

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES vtkm_filter vtkm_source
  ALL_BACKENDS
  USE_VTKM_JOB_POOL
  )
