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
  UnitTestExternalFacesFilter.cxx
  UnitTestExtractGeometryFilter.cxx
  UnitTestExtractPointsFilter.cxx
  UnitTestExtractStructuredFilter.cxx
  UnitTestGhostCellRemove.cxx
  UnitTestMaskFilter.cxx
  UnitTestMaskPointsFilter.cxx
  UnitTestThresholdFilter.cxx
  UnitTestThresholdPointsFilter.cxx
  )

set(libraries
  vtkm_filter_clean_grid
  vtkm_filter_entity_extraction
  )

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  USE_VTKM_JOB_POOL
)
