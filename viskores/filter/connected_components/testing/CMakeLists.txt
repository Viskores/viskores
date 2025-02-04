##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
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
