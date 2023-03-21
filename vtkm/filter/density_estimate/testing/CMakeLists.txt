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
  UnitTestContinuousScatterPlot.cxx
  UnitTestEntropyFilter.cxx
  UnitTestHistogramFilter.cxx
  UnitTestNDEntropyFilter.cxx
  UnitTestNDHistogramFilter.cxx
  UnitTestPartitionedDataSetHistogramFilter.cxx
  UnitTestStatisticsFilter.cxx
  UnitTestStatisticsFilterMPI.cxx
  )

set(unit_tests_device
  UnitTestParticleDensity.cxx # uses DescriptiveStatistcs worklet
  )

set(libraries
  vtkm_filter_density_estimate
  vtkm_source)

vtkm_unit_tests(
  SOURCES ${unit_tests}
  DEVICE_SOURCES ${unit_tests_device}
  LIBRARIES ${libraries}
  USE_VTKM_JOB_POOL
)

if (VTKm_ENABLE_MPI)
  set(mpi_unit_tests
    UnitTestStatisticsFilterMPI.cxx
  )
  vtkm_unit_tests(
    MPI
    DEVICE_SOURCES ${mpi_unit_tests}
    USE_VTKM_JOB_POOL
  )
endif()
