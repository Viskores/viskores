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


#if ((TARGET vtkm::cuda) OR (TARGET vtkm::kokkos_cuda))
#CUDA architecture has a limited amount of memory available for constants.The CUDA
#compiler uses this space to hold constants for some optimizations.However, for large
#kernels, the number of constants needed might be larger than the constant space
#available.For these conditions, you have to disable this form of optimization with
#the - Xptxas-- disable - optimizer - constants flags.
#TODO : Find a more elegant way to do this.Either figure out a way around this problem
# or add more general flags to vtkm_library / vtkm_unit_tests for sources with "large" kernels.
#  set(large_kernel_sources
#    UnitTestStreamlineFilter.cxx
#    UnitTestStreamSurfaceFilter.cxx
#    UnitTestWorkletParticleAdvection.cxx
#    UnitTestWorkletTemporalAdvection.cxx
#    )
#  set_source_files_properties(${large_kernel_sources} PROPERTIES
#    COMPILE_OPTIONS "--disable-optimizer-constants"
#    )
#endif()

vtkm_unit_tests(
  SOURCES ${unit_tests}
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
    MPI SOURCES ${mpi_unit_tests}
    LIBRARIES vtkm_filter vtkm_source vtkm_io
    ALL_BACKENDS
    USE_VTKM_JOB_POOL
  )
endif()
