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
  UnitTestFieldMetadata.cxx
  UnitTestFieldSelection.cxx
  UnitTestMapFieldMergeAverage.cxx
  UnitTestMapFieldPermutation.cxx
  UnitTestMultiBlockFilter.cxx
  UnitTestPartitionedDataSetFilters.cxx
)

set(libraries
  vtkm_filter
  vtkm_io
  vtkm_source
)

vtkm_unit_tests(
  DEVICE_SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  USE_VTKM_JOB_POOL
  )
