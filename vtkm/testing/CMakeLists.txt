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
  Testing.h
  VecTraitsTests.h
  )

VTKM_declare_headers(${headers})

set(unit_tests
  UnitTestBinaryPredicates.cxx
  UnitTestBinaryOperators.cxx
  UnitTestBounds.cxx
  UnitTestCellShape.cxx
  UnitTestDeprecated.cxx
  UnitTestExceptions.cxx
  UnitTestHash.cxx
  UnitTestList.cxx
  UnitTestListTag.cxx
  UnitTestMatrix.cxx
  UnitTestNewtonsMethod.cxx
  UnitTestNoAssert.cxx
  UnitTestPair.cxx
  UnitTestRange.cxx
  UnitTestTesting.cxx
  UnitTestTransform3D.cxx
  UnitTestTuple.cxx
  UnitTestTypeList.cxx
  UnitTestTypeListTag.cxx
  UnitTestTypes.cxx
  UnitTestTypeTraits.cxx
  UnitTestUnaryPredicates.cxx
  UnitTestVecAxisAlignedPointCoordinates.cxx
  UnitTestVecFromPortal.cxx
  UnitTestVecFromPortalPermute.cxx
  UnitTestVectorAnalysis.cxx
  UnitTestVecTraits.cxx
  UnitTestVecVariable.cxx
  )

# Unit tests that have device-specific code to be tested
set(unit_tests_device
  UnitTestAlgorithms.cxx
  UnitTestAtomic.cxx
  UnitTestGeometry.cxx
  UnitTestMath.cxx
  )


#suppress gcc note:
#variable tracking size limit exceeded with -fvar-tracking-assignments, retrying without
if (VTKM_COMPILER_IS_GNU)
  set_source_files_properties(UnitTestBounds.cxx PROPERTIES COMPILE_OPTIONS "-fno-var-tracking-assignments")
endif()

vtkm_unit_tests(SOURCES ${unit_tests})

vtkm_unit_tests(NAME UnitTests_vtkm_testing_device SOURCES ${unit_tests_device} ALL_BACKENDS)
