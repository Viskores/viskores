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
  UnitTestContourTreeUniformAugmentedFilter.cxx
  UnitTestContourTreeUniformAugmentedWorklet.cxx
  UnitTestContourTreeUniformDistributedFilter.cxx
  UnitTestDistributedBranchDecompositionFilter.cxx
  )

set(libraries
  vtkm_filter_scalar_topology
  vtkm_io
  vtkm_source
  )

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  ALL_BACKENDS # includes Worklet unit test and uses vtkm::cont::Algorithm
  USE_VTKM_JOB_POOL
)

if (VTKm_ENABLE_MPI)
  set(mpi_unit_tests
    UnitTestContourTreeUniformDistributedFilterMPI.cxx
    )
  vtkm_unit_tests(
    MPI SOURCES ${mpi_unit_tests}
    LIBRARIES vtkm_filter vtkm_source vtkm_io
    ALL_BACKENDS
    USE_VTKM_JOB_POOL
  )
endif()
