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
  ExplicitTestData.h
  MakeTestDataSet.h
  Testing.h
  TestingArrayHandles.h
  TestingArrayHandleMultiplexer.h
  TestingCellLocatorRectilinearGrid.h
  TestingCellLocatorTwoLevel.h
  TestingCellLocatorUniformGrid.h
  TestingColorTable.h
  TestingComputeRange.h
  TestingDeviceAdapter.h
  TestingDataSetExplicit.h
  TestingDataSetSingleType.h
  TestingFancyArrayHandles.h
  TestingImplicitFunction.h
  TestingPointLocatorSparseGrid.h
  TestingRuntimeDeviceConfiguration.h
  TestingSerialization.h
  TestingVirtualObjectHandle.h
  )

set(unit_tests
  UnitTestAlgorithm.cxx
  UnitTestArrayCopy.cxx
  UnitTestArrayExtractComponent.cxx
  UnitTestArrayGetValues.cxx
  UnitTestArrayHandleCartesianProduct.cxx
  UnitTestArrayHandleCompositeVector.cxx
  UnitTestArrayHandleConcatenate.cxx
  UnitTestArrayHandleCounting.cxx
  UnitTestArrayHandleDecorator.cxx
  UnitTestArrayHandleDiscard.cxx
  UnitTestArrayHandleExtractComponent.cxx
  UnitTestArrayHandleImplicit.cxx
  UnitTestArrayHandleIndex.cxx
  UnitTestArrayHandleOffsetsToNumComponents.cxx
  UnitTestArrayHandlePermutation.cxx
  UnitTestArrayHandleRandomStandardNormal.cxx
  UnitTestArrayHandleRandomUniformBits.cxx
  UnitTestArrayHandleRandomUniformReal.cxx
  UnitTestArrayHandleReverse.cxx
  UnitTestArrayHandleSwizzle.cxx
  UnitTestArrayHandleThreadSafety.cxx
  UnitTestArrayHandleTransform.cxx
  UnitTestArrayHandleUniformPointCoordinates.cxx
  UnitTestArrayHandleVirtual.cxx
  UnitTestArrayHandleXGCCoordinates.cxx
  UnitTestArrayPortalToIterators.cxx
  UnitTestArrayRangeCompute.cxx
  UnitTestCellLocatorChooser.cxx
  UnitTestCellLocatorGeneral.cxx
  UnitTestCellSet.cxx
  UnitTestCellSetExplicit.cxx
  UnitTestCellSetExtrude.cxx
  UnitTestCellSetPermutation.cxx
  UnitTestContTesting.cxx
  UnitTestDataSetBuilderCurvilinear.cxx
  UnitTestDataSetBuilderExplicit.cxx
  UnitTestDataSetBuilderRectilinear.cxx
  UnitTestDataSetBuilderUniform.cxx
  UnitTestDataSetConvertToExpected.cxx
  UnitTestDataSetPermutation.cxx
  UnitTestDataSetRectilinear.cxx
  UnitTestDataSetUniform.cxx
  UnitTestDeviceAdapterAlgorithmDependency.cxx
  UnitTestDeviceAdapterAlgorithmGeneral.cxx
  UnitTestDeviceSelectOnThreads.cxx
  UnitTestDynamicCellSet.cxx
  UnitTestError.cxx
  UnitTestFieldRangeCompute.cxx
  UnitTestInitialize.cxx
  UnitTestLogging.cxx
  UnitTestMergePartitionedDataSet.cxx
  UnitTestMoveConstructors.cxx
  UnitTestParticleArrayCopy.cxx
  UnitTestPartitionedDataSet.cxx
  UnitTestRuntimeDeviceInformation.cxx
  UnitTestRuntimeDeviceNames.cxx
  UnitTestScopedRuntimeDeviceTracker.cxx
  UnitTestStorageList.cxx
  UnitTestStorageListTag.cxx
  UnitTestTimer.cxx
  UnitTestToken.cxx
  UnitTestTryExecute.cxx
  UnitTestUnknownArrayHandle.cxx
  UnitTestUnknownCellSet.cxx
  UnitTestVariantArrayHandle.cxx
  )

set(library_sources
  TestEqualArrayHandles.cxx
  Testing.cxx
  )

set(library_sources_device
  )

vtkm_library(
  NAME vtkm_cont_testing
  SOURCES ${library_sources}
  DEVICE_SOURCES ${library_sources_device}
  HEADERS ${headers}
  )
target_link_libraries(vtkm_cont_testing PUBLIC vtkm_cont)

if(VTKm_ENABLE_TESTING)
  vtkm_unit_tests(SOURCES ${unit_tests} DEFINES VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)

#add distributed tests i.e.test to run with MPI
#if MPI is enabled.
  set(mpi_unit_tests
    UnitTestFieldRangeGlobalCompute.cxx
    UnitTestSerializationArrayHandle.cxx
    UnitTestSerializationDataSet.cxx
    )
  vtkm_unit_tests(MPI SOURCES ${mpi_unit_tests} DEFINES VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)
endif()
