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
  TestingMath.h
  TestingGeometry.h
  VecTraitsTests.h
  )

VTKM_declare_headers(${headers})

set(unit_tests
  UnitTestBinaryPredicates.cxx
  UnitTestBinaryOperators.cxx
  UnitTestBounds.cxx
  UnitTestCellShape.cxx
  UnitTestExceptions.cxx
  UnitTestHash.cxx
  UnitTestListTag.cxx
  UnitTestMath.cxx
  UnitTestMatrix.cxx
  UnitTestNewtonsMethod.cxx
  UnitTestNoAssert.cxx
  UnitTestPair.cxx
  UnitTestRange.cxx
  UnitTestTesting.cxx
  UnitTestTransform3D.cxx
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

VTKM_unit_tests(SOURCES ${unit_tests})
