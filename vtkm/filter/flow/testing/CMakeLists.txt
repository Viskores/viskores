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
  UnitTestStreamlineFilter.cxx
  UnitTestWorkletParticleAdvection.cxx
  UnitTestWorkletTemporalAdvection.cxx
  UnitTestStreamSurfaceFilter.cxx
  UnitTestStreamSurfaceWorklet.cxx
  )

set(libraries
  vtkm_filter_flow
  )

vtkm_unit_tests(
  DEVICE_SOURCES ${unit_tests}
  LIBRARIES ${libraries} vtkm_filter vtkm_io
  USE_VTKM_JOB_POOL
)

#add distributed tests i.e.test to run with MPI
#if MPI is enabled.
if (VTKm_ENABLE_MPI)
  set(mpi_unit_tests
    UnitTestParticleMessengerMPI.cxx
    UnitTestStreamlineFilterMPI.cxx
  )
  vtkm_unit_tests(
    MPI DEVICE_SOURCES ${mpi_unit_tests}
    LIBRARIES vtkm_filter vtkm_source vtkm_io
    ALL_BACKENDS
    USE_VTKM_JOB_POOL
  )
endif()
