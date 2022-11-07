##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##=============================================================================

set(headers
  SuperArcHelper.h
  TestingContourTreeUniformDistributedFilter.h
  VolumeHelper.h
  )

#vtkm_declare_headers(${headers})

set(unit_tests
  UnitTestContourTreeUniformFilter.cxx
  )

set(unit_tests_device
  UnitTestContourTreeUniformAugmentedFilter.cxx
  UnitTestContourTreeUniformAugmentedWorklet.cxx
  UnitTestContourTreeUniformDistributedFilter.cxx
  UnitTestDistributedBranchDecompositionFilter.cxx
  )

vtkm_unit_tests(
  SOURCES ${unit_tests}
  DEVICE_SOURCES ${unit_tests_device}
  USE_VTKM_JOB_POOL
)

if (VTKm_ENABLE_MPI)
  set(mpi_unit_tests
    )
  set(mpi_unit_tests_device
    UnitTestContourTreeUniformDistributedFilterMPI.cxx
    )
  vtkm_unit_tests(
    MPI
    SOURCES ${mpi_unit_tests}
    DEVICE_SOURCES ${mpi_unit_tests_device}
    LIBRARIES vtkm_filter vtkm_source vtkm_io
    USE_VTKM_JOB_POOL
  )
endif()
