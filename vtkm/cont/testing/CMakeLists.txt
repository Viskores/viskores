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
  TestingDeviceAdapter.h
  TestingRuntimeDeviceConfiguration.h
  TestingSerialization.h
  TestingVirtualObjectHandle.h
  )

set(unit_tests
  UnitTestArrayExtractComponent.cxx
  UnitTestArrayGetValues.cxx
  UnitTestArrayHandleCartesianProduct.cxx
  UnitTestArrayHandleCompositeVector.cxx
  UnitTestArrayHandleCounting.cxx
  UnitTestArrayHandleDiscard.cxx
  UnitTestArrayHandleIndex.cxx
  UnitTestArrayHandleOffsetsToNumComponents.cxx
  UnitTestArrayHandleRandomUniformBits.cxx
  UnitTestArrayHandleReverse.cxx
  UnitTestArrayHandleThreadSafety.cxx
  UnitTestArrayHandleUniformPointCoordinates.cxx
  UnitTestArrayHandleVirtual.cxx
  UnitTestArrayPortalToIterators.cxx
  UnitTestComputeRange.cxx
  UnitTestContTesting.cxx
  UnitTestDataSetBuilderCurvilinear.cxx
  UnitTestDataSetBuilderExplicit.cxx
  UnitTestDataSetBuilderRectilinear.cxx
  UnitTestDataSetBuilderUniform.cxx
  UnitTestDataSetConvertToExpected.cxx
  UnitTestDataSetExplicit.cxx
  UnitTestDataSetRectilinear.cxx
  UnitTestDataSetUniform.cxx
  UnitTestDeviceAdapterAlgorithmGeneral.cxx
  UnitTestDeviceSelectOnThreads.cxx
  UnitTestDynamicCellSet.cxx
  UnitTestError.cxx
  UnitTestFieldRangeCompute.cxx
  UnitTestInitialize.cxx
  UnitTestLateDeallocate.cxx
  UnitTestLogging.cxx
  UnitTestMergePartitionedDataSet.cxx
  UnitTestMoveConstructors.cxx
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

set(unit_tests_device
  UnitTestAlgorithm.cxx
  UnitTestArrayCopy.cxx
  UnitTestArrayHandle.cxx
  UnitTestArrayHandleConcatenate.cxx
  UnitTestArrayHandleConstant.cxx
  UnitTestArrayHandleCast.cxx
  UnitTestArrayHandleDecorator.cxx
  UnitTestArrayHandleExtractComponent.cxx
  UnitTestArrayHandleGroupVec.cxx
  UnitTestArrayHandleGroupVecVariable.cxx
  UnitTestArrayHandleImplicit.cxx
  UnitTestArrayHandleMultiplexer.cxx
  UnitTestArrayHandlePermutation.cxx
  UnitTestArrayHandleRandomStandardNormal.cxx
  UnitTestArrayHandleRandomUniformReal.cxx
  UnitTestArrayHandleRecombineVec.cxx
  UnitTestArrayHandleSOA.cxx
  UnitTestArrayHandleSwizzle.cxx
  UnitTestArrayHandleTransform.cxx
  UnitTestArrayHandleView.cxx
  UnitTestArrayHandleXGCCoordinates.cxx
  UnitTestArrayHandleZip.cxx
  UnitTestArrayRangeCompute.cxx
  UnitTestBitField.cxx
  UnitTestCellLocatorChooser.cxx
  UnitTestCellLocatorGeneral.cxx
  UnitTestCellLocatorRectilinearGrid.cxx
  UnitTestCellLocatorTwoLevel.cxx
  UnitTestCellLocatorUniformGrid.cxx
  UnitTestCellSet.cxx
  UnitTestCellSetExplicit.cxx
  UnitTestCellSetExtrude.cxx
  UnitTestCellSetPermutation.cxx
  UnitTestColorTable.cxx
  UnitTestDataSetPermutation.cxx
  UnitTestDataSetSingleType.cxx
  UnitTestDeviceAdapterAlgorithmDependency.cxx
  UnitTestImplicitFunction.cxx
  UnitTestParticleArrayCopy.cxx
  UnitTestPointLocatorSparseGrid.cxx
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
target_link_libraries(vtkm_cont_testing PUBLIC vtkm_cont vtkm_worklet)

if(VTKm_ENABLE_TESTING)
  vtkm_unit_tests(SOURCES ${unit_tests} DEVICE_SOURCES ${unit_tests_device})

#add distributed tests i.e.test to run with MPI
#if MPI is enabled.
  set(mpi_unit_tests
    UnitTestFieldRangeGlobalCompute.cxx
    UnitTestSerializationArrayHandle.cxx
    UnitTestSerializationDataSet.cxx
    )
  vtkm_unit_tests(MPI SOURCES ${mpi_unit_tests})
endif()
