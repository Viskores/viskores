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
  TestingContourTreeUniformDistributedFilter.h
)

set(unit_tests
  UnitTestCellAverageFilter.cxx
  UnitTestCellMeasuresFilter.cxx
  UnitTestCellSetConnectivityFilter.cxx
  UnitTestCleanGrid.cxx
  UnitTestClipWithFieldFilter.cxx
  UnitTestClipWithImplicitFunctionFilter.cxx
  UnitTestContourFilter.cxx
  UnitTestContourFilterNormals.cxx
  UnitTestContourTreeUniformFilter.cxx
  UnitTestContourTreeUniformAugmentedFilter.cxx
  UnitTestContourTreeUniformDistributedFilter.cxx
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
  UnitTestGradientExplicit.cxx
  UnitTestGradientUniform.cxx
  UnitTestGhostCellClassify.cxx
  UnitTestGhostCellRemove.cxx
  UnitTestHistogramFilter.cxx
  UnitTestImageConnectivityFilter.cxx
  UnitTestImageDifferenceFilter.cxx
  UnitTestImageMedianFilter.cxx
  UnitTestLagrangianFilter.cxx
  UnitTestLagrangianStructuresFilter.cxx
  UnitTestMapFieldMergeAverage.cxx
  UnitTestMapFieldPermutation.cxx
  UnitTestMaskFilter.cxx
  UnitTestMaskPointsFilter.cxx
  UnitTestMeshQualityFilter.cxx
  UnitTestNDEntropyFilter.cxx
  UnitTestNDHistogramFilter.cxx
  UnitTestParticleDensity.cxx
  UnitTestPartitionedDataSetFilters.cxx
  UnitTestPartitionedDataSetHistogramFilter.cxx
  UnitTestPointAverageFilter.cxx
  UnitTestPointAverageCellSetExtrude.cxx
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

set(libraries
  vtkm_filter
  vtkm_io
  vtkm_source
)

if (VTKm_ENABLE_RENDERING)
  list(APPEND libraries vtkm_rendering)

  list(APPEND unit_tests
    RegressionTestContourFilter.cxx
    RegressionTestPointTransform.cxx
    RegressionTestSplitSharpEdges.cxx
    RegressionTestStreamline.cxx
    RegressionTestSurfaceNormals.cxx
  )
endif()

if ((TARGET vtkm::cuda) OR (TARGET vtkm::kokkos_cuda))
  # CUDA architecture has a limited amount of memory available for constants. The CUDA
  # compiler uses this space to hold constants for some optimizations. However, for large
  # kernels, the number of constants needed might be larger than the constant space
  # available. For these conditions, you have to disable this form of optimization with
  # the -Xptxas --disable-optimizer-constants flags.
  # TODO: Find a more elegant way to do this. Either figure out a way around this problem
  # or add more general flags to vtkm_library/vtkm_unit_tests for sources with "large" kernels.
  set(large_kernel_sources
    RegressionTestStreamline.cxx
    UnitTestLagrangianFilter.cxx
    UnitTestStreamlineFilter.cxx
    UnitTestStreamSurfaceFilter.cxx
    )
  set_source_files_properties(${large_kernel_sources} PROPERTIES
    COMPILE_OPTIONS "-Xptxas;--disable-optimizer-constants"
    )
endif()

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  ALL_BACKENDS
  USE_VTKM_JOB_POOL
  )

# add distributed tests i.e. test to run with MPI
# if MPI is enabled.
if (VTKm_ENABLE_MPI)
  set(mpi_unit_tests
    UnitTestContourTreeUniformDistributedFilterMPI.cxx
    UnitTestParticleMessengerMPI.cxx
    UnitTestStreamlineFilterMPI.cxx
  )
  vtkm_unit_tests(
    MPI SOURCES ${mpi_unit_tests}
    LIBRARIES vtkm_filter vtkm_source vtkm_io
    ALL_BACKENDS
    USE_VTKM_JOB_POOL
  )
endif()
