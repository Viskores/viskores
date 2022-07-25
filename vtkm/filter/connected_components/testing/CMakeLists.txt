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
  UnitTestCellSetConnectivityFilter.cxx # uses vtkm::cont::Algorithm
  UnitTestGraphConnectivityWorklet.cxx
  )

set(libraries
  vtkm_filter_contour
  vtkm_filter_connected_components
  vtkm_source
  )

vtkm_unit_tests(
  SOURCES ${unit_tests}
  DEVICE_SOURCES ${unit_tests_device}
  LIBRARIES ${libraries}
  USE_VTKM_JOB_POOL
)
