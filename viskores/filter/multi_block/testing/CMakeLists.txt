##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(libraries
  viskores_filter_multi_block
  viskores_filter_geometry_refinement
  viskores_source
  viskores_rendering
  viskores_rendering_testing
  )

set(unit_tests
  RenderTestAmrArrays.cxx
  )
set(unit_tests_device
  UnitTestMergeDataSetsFilter.cxx
  )

viskores_unit_tests(
  SOURCES ${unit_tests}
  DEVICE_SOURCES ${unit_tests_device}
  LIBRARIES ${libraries}
  USE_VISKORES_JOB_POOL
  )
