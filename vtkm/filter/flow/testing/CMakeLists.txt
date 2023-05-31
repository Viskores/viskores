##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(filter_unit_tests
  UnitTestLagrangianFilter.cxx
  UnitTestLagrangianStructuresFilter.cxx
  UnitTestStreamlineFilter.cxx
  UnitTestStreamlineFilterWarpX.cxx
  UnitTestStreamSurfaceFilter.cxx
  )
set(worklet_unit_tests
  UnitTestWorkletParticleAdvection.cxx
  UnitTestWorkletTemporalAdvection.cxx
  UnitTestStreamSurfaceWorklet.cxx
  )

if (TARGET vtkm_rendering_testing)
  list(APPEND filter_unit_tests
    RenderTestStreamline.cxx
    )
endif()

vtkm_unit_tests(
  SOURCES ${filter_unit_tests}
  DEVICE_SOURCES ${worklet_unit_tests}
  USE_VTKM_JOB_POOL
  )

#add distributed tests i.e.test to run with MPI
#if MPI is enabled.
if (VTKm_ENABLE_MPI)
  set(mpi_unit_tests
    UnitTestAdvectionAsynchronousMPI.cxx
    UnitTestAdvectionSynchronousMPI.cxx
    UnitTestParticleMessengerMPI.cxx
    UnitTestPathlineAsynchronousMPI.cxx
    UnitTestPathlineSynchronousMPI.cxx
    UnitTestStreamlineAMRMPI.cxx
    UnitTestStreamlineAsynchronousMPI.cxx
    UnitTestStreamlineSynchronousMPI.cxx
    )
  set(mpi_helpers
    TestingFlow.cxx
    TestingFlow.h
    )
  set_source_files_properties(${mpi_helpers} PROPERTIES NOT_A_TEST TRUE)
  vtkm_unit_tests(
    MPI
    SOURCES ${mpi_unit_tests} ${mpi_helpers}
    USE_VTKM_JOB_POOL
  )
endif()
