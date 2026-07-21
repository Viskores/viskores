##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(unit_tests
  UnitTestImageConnectivityFilter.cxx
  )

set(unit_tests_device
  UnitTestCellSetConnectivityFilter.cxx # uses viskores::cont::Algorithm
  UnitTestGraphConnectivityWorklet.cxx
  )

set(libraries
  viskores_filter_contour
  viskores_filter_connected_components
  viskores_source
  )

viskores_unit_tests(
  SOURCES ${unit_tests}
  DEVICE_SOURCES ${unit_tests_device}
  LIBRARIES ${libraries}
  USE_VISKORES_JOB_POOL
)
