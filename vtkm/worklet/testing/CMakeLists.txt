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
  UnitTestAverageByKey.cxx
  UnitTestBoundingIntervalHierarchy.cxx
  UnitTestCellAverage.cxx
  UnitTestCellDeepCopy.cxx
  UnitTestCellGradient.cxx
  UnitTestCellSetConnectivity.cxx
  UnitTestCellSetDualGraph.cxx
  UnitTestCellMeasure.cxx
  UnitTestClipping.cxx
  UnitTestContour.cxx
  UnitTestContourTreeUniform.cxx
  UnitTestContourTreeUniformAugmented.cxx
  UnitTestCoordinateSystemTransform.cxx
  UnitTestCosmoTools.cxx
  UnitTestCrossProduct.cxx
  UnitTestDotProduct.cxx
  UnitTestExternalFaces.cxx
  UnitTestExtractGeometry.cxx
  UnitTestExtractPoints.cxx
  UnitTestExtractStructured.cxx
  UnitTestFieldHistogram.cxx
  UnitTestFieldStatistics.cxx
  UnitTestGraphConnectivity.cxx
  UnitTestInnerJoin.cxx
  UnitTestImageConnectivity.cxx
  UnitTestKdTreeBuildNNS.cxx
  UnitTestKeys.cxx
  UnitTestMagnitude.cxx
  UnitTestMask.cxx
  UnitTestMaskIndices.cxx
  UnitTestMaskPoints.cxx
  UnitTestMaskSelect.cxx
  UnitTestNormalize.cxx
  UnitTestNDimsEntropy.cxx
  UnitTestNDimsHistogram.cxx
  UnitTestNDimsHistMarginalization.cxx
  UnitTestOrientNormals.cxx
  UnitTestParticleAdvection.cxx
  UnitTestPointElevation.cxx
  UnitTestPointGradient.cxx
  UnitTestPointTransform.cxx
  UnitTestProbe.cxx
  UnitTestRemoveUnusedPoints.cxx
  UnitTestScalarsToColors.cxx
  UnitTestScatterAndMask.cxx
  UnitTestScatterCounting.cxx
  UnitTestScatterPermutation.cxx
  UnitTestSplatKernels.cxx
  UnitTestSplitSharpEdges.cxx
  UnitTestStreamingSine.cxx
  UnitTestStreamLineUniformGrid.cxx
  UnitTestStreamSurface.cxx
  UnitTestSurfaceNormals.cxx
  UnitTestTemporalAdvection.cxx
  UnitTestTetrahedralize.cxx
  UnitTestThreshold.cxx
  UnitTestThresholdPoints.cxx
  UnitTestTriangleWinding.cxx
  UnitTestTriangulate.cxx
  UnitTestTube.cxx
  UnitTestWholeCellSetIn.cxx
  UnitTestWorkletMapField.cxx
  UnitTestWorkletMapFieldExecArg.cxx
  UnitTestWorkletMapFieldWholeArray.cxx
  UnitTestWorkletMapFieldWholeArrayAtomic.cxx
  UnitTestWorkletMapPointNeighborhood.cxx
  UnitTestWorkletMapTopologyExplicit.cxx
  UnitTestWorkletMapTopologyUniform.cxx
  UnitTestWorkletReduceByKey.cxx
  UnitTestVertexClustering.cxx
  UnitTestWarpScalar.cxx
  UnitTestWarpVector.cxx
  UnitTestWaveletCompressor.cxx
  UnitTestZFPCompressor.cxx
 )


vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES vtkm_source vtkm_worklet vtkm_filter
  ALL_BACKENDS
  USE_VTKM_JOB_POOL
  )
if (TARGET vtkm::cuda)
  if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA" AND
     CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 10.0.0)
    set(problematic_cuda_srcs
      "${CMAKE_CURRENT_BINARY_DIR}/UnitTestParticleAdvection.cu"
      "${CMAKE_CURRENT_BINARY_DIR}/UnitTestBoundingIntervalHierarchy.cu"
    )
    set_source_files_properties(${problematic_cuda_srcs} PROPERTIES COMPILE_FLAGS "-Xptxas --opt-level=0")
  endif()
endif()
