##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  TestingTaskTiling.h
  ThreadIndicesTesting.h
  )

viskores_declare_headers(${headers})

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
viskores_unit_tests(SOURCES ${unit_tests})
