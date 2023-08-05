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
  UnitTestCoordinateSystemTransform.cxx
  UnitTestFieldToColors.cxx
  UnitTestGenerateIds.cxx
  UnitTestPointElevationFilter.cxx
  UnitTestPointTransform.cxx
  UnitTestWarpFilter.cxx
  UnitTestLogValues.cxx
  UnitTestCompositeVectors.cxx
  )

set(libraries
  vtkm_filter_field_transform
  vtkm_filter_vector_analysis
  vtkm_source)

if (VTKm_ENABLE_RENDERING)
  list(APPEND libraries
    vtkm_rendering_testing
    vtkm_rendering)

  list(APPEND unit_tests
    RenderTestPointTransform.cxx
    )
endif()

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  USE_VTKM_JOB_POOL
)
