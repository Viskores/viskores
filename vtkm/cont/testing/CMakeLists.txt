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
  TestingArrayHandleVirtualCoordinates.h
  TestingCellLocatorRectilinearGrid.h
  TestingCellLocatorUniformBins.h
  TestingCellLocatorUniformGrid.h
  TestingColorTable.h
  TestingComputeRange.h
  TestingDeviceAdapter.h
  TestingDataSetExplicit.h
  TestingDataSetSingleType.h
  TestingFancyArrayHandles.h
  TestingImplicitFunction.h
  TestingPointLocatorUniformGrid.h
  TestingSerialization.h
  TestingVirtualObjectHandle.h
  )

vtkm_declare_headers(${headers})

set(unit_tests
  UnitTestAlgorithm.cxx
  UnitTestArrayCopy.cxx
  UnitTestArrayGetValues.cxx
  UnitTestArrayHandleCartesianProduct.cxx
  UnitTestArrayHandleCompositeVector.cxx
  UnitTestArrayHandleCounting.cxx
  UnitTestArrayHandleDecorator.cxx
  UnitTestArrayHandleDiscard.cxx
  UnitTestArrayHandleExtractComponent.cxx
  UnitTestArrayHandleExtrude.cxx
  UnitTestArrayHandleImplicit.cxx
  UnitTestArrayHandleIndex.cxx
  UnitTestArrayHandleReverse.cxx
  UnitTestArrayHandlePermutation.cxx
  UnitTestArrayHandleSwizzle.cxx
  UnitTestArrayHandleTransform.cxx
  UnitTestArrayHandleUniformPointCoordinates.cxx
  UnitTestArrayHandleConcatenate.cxx
  UnitTestArrayHandleVirtual.cxx
  UnitTestArrayPortalToIterators.cxx
  UnitTestCellLocatorGeneral.cxx
  UnitTestCellSet.cxx
  UnitTestCellSetExplicit.cxx
  UnitTestCellSetExtrude.cxx
  UnitTestCellSetPermutation.cxx
  UnitTestContTesting.cxx
  UnitTestDataSetBuilderExplicit.cxx
  UnitTestDataSetBuilderRectilinear.cxx
  UnitTestDataSetBuilderUniform.cxx
  UnitTestDataSetPermutation.cxx
  UnitTestDataSetRectilinear.cxx
  UnitTestDataSetUniform.cxx
  UnitTestDeviceAdapterAlgorithmDependency.cxx
  UnitTestDeviceAdapterAlgorithmGeneral.cxx
  UnitTestDynamicCellSet.cxx
  UnitTestError.cxx
  UnitTestFieldRangeCompute.cxx
  UnitTestInitialize.cxx
  UnitTestLogging.cxx
  UnitTestMoveConstructors.cxx
  UnitTestPartitionedDataSet.cxx
  UnitTestRuntimeDeviceInformation.cxx
  UnitTestRuntimeDeviceNames.cxx
  UnitTestScopedRuntimeDeviceTracker.cxx
  UnitTestStorageBasic.cxx
  UnitTestStorageImplicit.cxx
  UnitTestStorageList.cxx
  UnitTestStorageListTag.cxx
  UnitTestTimer.cxx
  UnitTestTryExecute.cxx
  UnitTestVariantArrayHandle.cxx
  )

vtkm_unit_tests(SOURCES ${unit_tests} DEFINES VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)

# add distributed tests i.e. test to run with MPI
# if MPI is enabled.
set(mpi_unit_tests
  UnitTestFieldRangeGlobalCompute.cxx
  UnitTestSerializationArrayHandle.cxx
  UnitTestSerializationDataSet.cxx
  )
vtkm_unit_tests(MPI SOURCES ${mpi_unit_tests} DEFINES VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)
