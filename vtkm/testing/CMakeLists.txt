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
  Testing.h
  TestingMath.h
  TestingGeometry.h
  OptionParser.h
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
