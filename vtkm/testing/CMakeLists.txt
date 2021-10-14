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

if(VTKm_ENABLE_TESTING)
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
    UnitTestVecFlat.cxx
    UnitTestVecTraits.cxx
    UnitTestVecVariable.cxx
    )

  # Unit tests that have device-specific code to be tested
  set(unit_tests_device
    UnitTestAlgorithms.cxx
    UnitTestAtomic.cxx
    UnitTestGeometry.cxx
    UnitTestLowerBound.cxx
    UnitTestMath.cxx
    UnitTestUpperBound.cxx
    )


  #variable tracking size limit exceeded with -fvar-tracking-assignments, retrying without
  set_source_files_properties(UnitTestBounds.cxx PROPERTIES
    COMPILE_OPTIONS "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-fno-var-tracking-assignments>")

  vtkm_unit_tests(SOURCES ${unit_tests})

  vtkm_unit_tests(NAME UnitTests_vtkm_testing_device SOURCES ${unit_tests_device} ALL_BACKENDS)
endif()
