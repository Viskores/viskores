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
  TestingDeviceAdapter.h
  TestingRuntimeDeviceConfiguration.h
  TestingSerialization.h
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
  UnitTestArrayPortalFromIterators.cxx
  UnitTestArrayPortalToIterators.cxx
  UnitTestBuffer.cxx
  UnitTestComputeRange.cxx
  UnitTestControlSignatureTag.cxx
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
  UnitTestIteratorFromArrayPortal.cxx
  UnitTestLateDeallocate.cxx
  UnitTestLogging.cxx
  UnitTestMergePartitionedDataSet.cxx
  UnitTestMoveConstructors.cxx
  UnitTestPartitionedDataSet.cxx
  UnitTestRuntimeConfigurationOptions.cxx
  UnitTestRuntimeDeviceInformation.cxx
  UnitTestRuntimeDeviceNames.cxx
  UnitTestScopedRuntimeDeviceTracker.cxx
  UnitTestStorageList.cxx
  UnitTestStorageListTag.cxx
  UnitTestTimer.cxx
  UnitTestToken.cxx
  UnitTestTryExecute.cxx
  UnitTestTypeCheckArray.cxx
  UnitTestTypeCheckCellSet.cxx
  UnitTestTypeCheckExecObject.cxx
  UnitTestTypeCheckKeys.cxx
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
  UnitTestCellLocatorPartitioned.cxx
  UnitTestCellLocatorRectilinearGrid.cxx
  UnitTestCellLocatorTwoLevel.cxx
  UnitTestCellLocatorUniformGrid.cxx
  UnitTestCellSet.cxx
  UnitTestCellSetExplicit.cxx
  UnitTestCellSetPermutation.cxx
  UnitTestColorTable.cxx
  UnitTestDataSetPermutation.cxx
  UnitTestDataSetSingleType.cxx
  UnitTestDeviceAdapterAlgorithmDependency.cxx
  UnitTestImplicitFunction.cxx
  UnitTestParticleArrayCopy.cxx
  UnitTestPointLocatorSparseGrid.cxx
  UnitTestTransportArrayIn.cxx
  UnitTestTransportArrayInOut.cxx
  UnitTestTransportArrayOut.cxx
  UnitTestTransportCellSetIn.cxx
  UnitTestTransportExecObject.cxx
  UnitTestTransportWholeArray.cxx
  )
if(TARGET vtkm_filter_field_conversion)
  list(APPEND unit_tests_device
    UnitTestCellSetExtrude.cxx
    )
endif()

if (NOT VTKm_NO_DEPRECATED_VIRTUAL)
  list(APPEND unit_tests_device
    UnitTestVirtualObjectHandle.cxx
    )
endif()

vtkm_unit_tests(SOURCES ${unit_tests} DEVICE_SOURCES ${unit_tests_device})

#add distributed tests i.e.test to run with MPI
#if MPI is enabled.
set(mpi_unit_tests
  UnitTestFieldRangeGlobalCompute.cxx
  UnitTestSerializationArrayHandle.cxx
  UnitTestSerializationDataSet.cxx
  )
vtkm_unit_tests(MPI SOURCES ${mpi_unit_tests})
