##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  TestingContourTreeUniformDistributedFilter.h
)

set(unit_tests
  UnitTestContourTreeUniformFilter.cxx
  UnitTestContourTreeUniformAugmentedFilter.cxx
  UnitTestContourTreeUniformDistributedFilter.cxx
  UnitTestFieldMetadata.cxx
  UnitTestFieldSelection.cxx
  UnitTestLagrangianFilter.cxx
  UnitTestLagrangianStructuresFilter.cxx
  UnitTestMapFieldMergeAverage.cxx
  UnitTestMapFieldPermutation.cxx
  UnitTestMIRFilter.cxx
  UnitTestMultiBlockFilter.cxx
  UnitTestPartitionedDataSetFilters.cxx
  UnitTestProbe.cxx
  UnitTestStreamlineFilter.cxx
  UnitTestStreamSurfaceFilter.cxx
)

#Taking too long to compile with HIPCC
if(HIP IN_LIST Kokkos_DEVICES)
  list(REMOVE_ITEM unit_tests
       UnitTestLagrangianFilter.cxx
       UnitTestLagrangianStructuresFilter.cxx
       UnitTestStreamlineFilter.cxx
       UnitTestStreamSurfaceFilter.cxx
  )
endif()

set(libraries
  vtkm_filter
  vtkm_io
  vtkm_source
)

if (VTKm_ENABLE_RENDERING)
  list(APPEND libraries vtkm_rendering vtkm_rendering_testing)

  list(APPEND unit_tests
    RenderTestAmrArrays.cxx
    RenderTestStreamline.cxx
  )
endif()

if ((TARGET vtkm::cuda) OR (TARGET vtkm::kokkos_cuda))
#CUDA architecture has a limited amount of memory available for constants.The CUDA
#compiler uses this space to hold constants for some optimizations.However, for large
#kernels, the number of constants needed might be larger than the constant space
#available.For these conditions, you have to disable this form of optimization with
#the - Xptxas-- disable - optimizer - constants flags.
#TODO : Find a more elegant way to do this.Either figure out a way around this problem
# or add more general flags to vtkm_library / vtkm_unit_tests for sources with "large" kernels.
  set(large_kernel_sources
    RegressionTestStreamline.cxx
    UnitTestLagrangianFilter.cxx
    UnitTestStreamlineFilter.cxx
    UnitTestStreamSurfaceFilter.cxx
    )
  set_source_files_properties(${large_kernel_sources} PROPERTIES
    COMPILE_OPTIONS "-Xptxas;--disable-optimizer-constants"
    )
endif()

vtkm_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  ALL_BACKENDS
  USE_VTKM_JOB_POOL
  )

#add distributed tests i.e.test to run with MPI
#if MPI is enabled.
if (VTKm_ENABLE_MPI)
  set(mpi_unit_tests
    UnitTestContourTreeUniformDistributedFilterMPI.cxx
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
