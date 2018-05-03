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

set(headers
  ExplicitTestData.h
  MakeTestDataSet.h
  Testing.h
  TestingArrayHandles.h
  TestingArrayHandleVirtualCoordinates.h
  TestingCellLocatorTwoLevelUniformGrid.h
  TestingColorTable.h
  TestingComputeRange.h
  TestingDeviceAdapter.h
  TestingDataSetExplicit.h
  TestingDataSetSingleType.h
  TestingFancyArrayHandles.h
  TestingImplicitFunction.h
  TestingPointLocatorUniformGrid.h
  TestingVirtualObjectHandle.h
  )

vtkm_declare_headers(${headers})

set(unit_tests
  UnitTestAlgorithm.cxx
  UnitTestArrayCopy.cxx
  UnitTestArrayHandleCartesianProduct.cxx
  UnitTestArrayHandleCompositeVector.cxx
  UnitTestArrayHandleCounting.cxx
  UnitTestArrayHandleDiscard.cxx
  UnitTestArrayHandleExtractComponent.cxx
  UnitTestArrayHandleImplicit.cxx
  UnitTestArrayHandleIndex.cxx
  UnitTestArrayHandleReverse.cxx
  UnitTestArrayHandlePermutation.cxx
  UnitTestArrayHandleSwizzle.cxx
  UnitTestArrayHandleTransform.cxx
  UnitTestArrayHandleUniformPointCoordinates.cxx
  UnitTestArrayHandleConcatenate.cxx
  UnitTestArrayPortalToIterators.cxx
  UnitTestCellLocator.cxx
  UnitTestCellSetExplicit.cxx
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
  UnitTestDynamicArrayHandle.cxx
  UnitTestDynamicCellSet.cxx
  UnitTestFieldRangeCompute.cxx
  UnitTestMultiBlock.cxx
  UnitTestRuntimeDeviceInformation.cxx
  UnitTestStorageBasic.cxx
  UnitTestStorageImplicit.cxx
  UnitTestStorageListTag.cxx
  UnitTestTimer.cxx
  UnitTestTryExecute.cxx
  )

vtkm_unit_tests(SOURCES ${unit_tests})


# add distributed tests i.e. test to run with MPI
# if MPI is enabled.
set(mpi_unit_tests
  UnitTestFieldRangeGlobalCompute.cxx)
vtkm_unit_tests(MPI SOURCES ${mpi_unit_tests})
