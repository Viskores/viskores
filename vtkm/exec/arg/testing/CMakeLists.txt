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
  ThreadIndicesTesting.h
  )

vtkm_declare_headers(${headers})

set(unit_tests
  UnitTestExecutionSignatureTag.cxx
  UnitTestFetchArrayDirectIn.cxx
  UnitTestFetchArrayDirectInOut.cxx
  UnitTestFetchArrayDirectOut.cxx
  UnitTestFetchArrayNeighborhoodIn.cxx
  UnitTestFetchArrayTopologyMapIn.cxx
  UnitTestFetchExecObject.cxx
  UnitTestFetchWorkIndex.cxx
  )

vtkm_unit_tests(SOURCES ${unit_tests})
