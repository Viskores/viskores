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
  UnitTestLagrangianFilter.cxx
  UnitTestLagrangianStructuresFilter.cxx
  UnitTestMapFieldMergeAverage.cxx
  UnitTestMapFieldPermutation.cxx
  UnitTestMultiBlockFilter.cxx
  UnitTestPartitionedDataSetFilters.cxx
)

set(libraries
  vtkm_filter
  vtkm_filter_extra
  vtkm_io
  vtkm_source
)

if (VTKm_ENABLE_RENDERING)
  list(APPEND libraries vtkm_rendering vtkm_rendering_testing)

  list(APPEND unit_tests
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
    )
  set_source_files_properties(${large_kernel_sources} PROPERTIES
    COMPILE_OPTIONS "-Xptxas;--disable-optimizer-constants"
    )
endif()

vtkm_unit_tests(
  DEVICE_SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  USE_VTKM_JOB_POOL
  )
