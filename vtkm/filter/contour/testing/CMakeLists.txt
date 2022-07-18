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
  UnitTestClipWithFieldFilter.cxx
  UnitTestClipWithImplicitFunctionFilter.cxx
  UnitTestContourFilterNormals.cxx
)

set(unit_tests_device
  UnitTestContourFilter.cxx # Algorithm used, needs device compiler
  UnitTestMIRFilter.cxx # Algorithm used, needs device compiler
  )

set(libraries
  vtkm_filter_clean_grid
  vtkm_filter_contour
  vtkm_filter_field_transform
  vtkm_filter_geometry_refinement
  vtkm_io
  vtkm_source
)

if (VTKm_ENABLE_RENDERING)
  list(APPEND libraries
    vtkm_rendering_testing
    vtkm_rendering)

  list(APPEND unit_tests
    RenderTestContourFilter.cxx
    RenderTestSliceFilter.cxx
  )
endif()

vtkm_unit_tests(
  SOURCES ${unit_tests}
  DEVICE_SOURCES ${unit_tests_device}
  LIBRARIES ${libraries}
  USE_VTKM_JOB_POOL
)
