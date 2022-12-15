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
  GenerateTestDataSets.h
  TestingContourTreeUniformDistributedLoadArrays.h
  )

set(unit_tests
  UnitTestAverageByKey.cxx
  UnitTestBoundingIntervalHierarchy.cxx
  UnitTestCellDeepCopy.cxx
  UnitTestContourTreeUniformDistributed.cxx
  UnitTestCosmoTools.cxx
  UnitTestDescriptiveStatistics.cxx
  UnitTestDispatcherBase.cxx
  UnitTestFieldStatistics.cxx
  UnitTestKeys.cxx
  UnitTestMaskIndices.cxx
  UnitTestMaskSelect.cxx
  UnitTestNormalize.cxx
  UnitTestNDimsHistMarginalization.cxx
  UnitTestScalarsToColors.cxx
  UnitTestScatterAndMask.cxx
  UnitTestScatterCounting.cxx
  UnitTestScatterPermutation.cxx
  UnitTestSplatKernels.cxx
  UnitTestScatterAndMaskWithTopology.cxx
  UnitTestStreamLineUniformGrid.cxx
  UnitTestTriangleWinding.cxx
  UnitTestWholeCellSetIn.cxx
  UnitTestWorkletMapField.cxx
  UnitTestWorkletMapField3d.cxx
  UnitTestWorkletMapFieldExecArg.cxx
  UnitTestWorkletMapFieldWholeArray.cxx
  UnitTestWorkletMapFieldWholeArrayAtomic.cxx
  UnitTestWorkletMapPointNeighborhood.cxx
  UnitTestWorkletMapTopologyExplicit.cxx
  UnitTestWorkletMapTopologyUniform.cxx
  UnitTestWorkletReduceByKey.cxx
  UnitTestWaveletCompressor.cxx
 )

vtkm_unit_tests(
  DEVICE_SOURCES ${unit_tests}
  LIBRARIES vtkm_source vtkm_worklet vtkm_filter vtkm_io
  USE_VTKM_JOB_POOL
  )
if (TARGET vtkm_cuda)
  if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA" AND
     CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 10.0.0)
    set(problematic_cuda_srcs
      "${CMAKE_CURRENT_BINARY_DIR}/UnitTestParticleAdvection.cu"
      "${CMAKE_CURRENT_BINARY_DIR}/UnitTestBoundingIntervalHierarchy.cu"
    )
    set_source_files_properties(${problematic_cuda_srcs} PROPERTIES COMPILE_FLAGS "-Xptxas --opt-level=0")
  endif()
endif()
