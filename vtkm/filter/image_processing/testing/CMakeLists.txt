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
  RenderTestComputeMoments.cxx
  UnitTestImageDifferenceFilter.cxx
  UnitTestImageMedianFilter.cxx
  )

set(libraries
  vtkm_filter_image_processing
  vtkm_source
  vtkm_rendering
  vtkm_rendering_testing
)

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  USE_VTKM_JOB_POOL
)
