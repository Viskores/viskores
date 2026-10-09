##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(libraries
  viskores_filter_core
  viskores_filter_vector_analysis
  )

set(unit_tests
  UnitTestContourUncertainGaussianCorrelated.cxx
  UnitTestContourUncertainGaussianIndependent.cxx
  UnitTestContourUncertainUniform.cxx
  UnitTestFiberUncertainUniform.cxx
  )

viskores_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  USE_VISKORES_JOB_POOL
  )
