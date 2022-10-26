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
  TestingTaskTiling.h
  ThreadIndicesTesting.h
  )

vtkm_declare_headers(${headers})

set(unit_tests
  UnitTestCellDerivative.cxx
  UnitTestCellEdgeFace.cxx
  UnitTestCellInterpolate.cxx
  UnitTestErrorMessageBuffer.cxx
  UnitTestExecutionSignatureTag.cxx
  UnitTestFetchArrayDirectIn.cxx
  UnitTestFetchArrayDirectIn3d.cxx
  UnitTestFetchArrayDirectInOut.cxx
  UnitTestFetchArrayDirectOut.cxx
  UnitTestFetchArrayNeighborhoodIn.cxx
  UnitTestFetchArrayTopologyMapIn.cxx
  UnitTestFetchExecObject.cxx
  UnitTestFetchWorkIndex.cxx
  UnitTestParametricCoordinates.cxx
  UnitTestTaskSingular.cxx
  UnitTestVariant.cxx
  UnitTestWorkletInvokeFunctor.cxx
  )
vtkm_unit_tests(SOURCES ${unit_tests})
