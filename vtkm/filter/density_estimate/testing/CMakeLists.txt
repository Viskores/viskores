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
  UnitTestEntropyFilter.cxx
  UnitTestHistogramFilter.cxx
  UnitTestNDEntropyFilter.cxx
  UnitTestNDHistogramFilter.cxx
  UnitTestPartitionedDataSetHistogramFilter.cxx
  UnitTestParticleDensity.cxx)

set(libraries
  vtkm_filter_density_estimate
  vtkm_source)

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  ALL_BACKENDS # UnitTestParticleDensity.cxx uses DescriptiveStatistcs worklet
  USE_VTKM_JOB_POOL
)
