##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(libraries
  vtkm_filter_multi_block
  vtkm_filter_geometry_refinement
  vtkm_source
  vtkm_rendering
  vtkm_rendering_testing
  )

set(unit_tests
  RenderTestAmrArrays.cxx
  )
set(unit_tests_device
  UnitTestMergeDataSetsFilter.cxx
  )

vtkm_unit_tests(
  SOURCES ${unit_tests}
  DEVICE_SOURCES ${unit_tests_device}
  LIBRARIES ${libraries}
  USE_VTKM_JOB_POOL
  )
