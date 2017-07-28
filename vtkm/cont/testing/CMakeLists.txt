##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 Sandia Corporation.
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
  TestingComputeRange.h
  TestingDeviceAdapter.h
  TestingDataSetExplicit.h
  TestingDataSetSingleType.h
  TestingFancyArrayHandles.h
  TestingImplicitFunction.h
  TestingVirtualObjectCache.h
  )

vtkm_declare_headers(${headers})

set(unit_tests
  UnitTestArrayHandleCartesianProduct.cxx
  UnitTestArrayHandleCompositeVector.cxx
  UnitTestArrayHandleCounting.cxx
  UnitTestArrayHandleDiscard.cxx
  UnitTestArrayHandleImplicit.cxx
  UnitTestArrayHandleIndex.cxx
  UnitTestArrayHandleReverse.cxx
  UnitTestArrayHandlePermutation.cxx
  UnitTestArrayHandleSwizzle.cxx
  UnitTestArrayHandleTransform.cxx
  UnitTestArrayHandleUniformPointCoordinates.cxx
  UnitTestArrayHandleConcatenate.cxx
  UnitTestArrayPortalToIterators.cxx
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
  UnitTestRuntimeDeviceInformation.cxx
  UnitTestStorageBasic.cxx
  UnitTestStorageImplicit.cxx
  UnitTestStorageListTag.cxx
  UnitTestTimer.cxx
  UnitTestTryExecute.cxx
  )

vtkm_unit_tests(SOURCES ${unit_tests})
