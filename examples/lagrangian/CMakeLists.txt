##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if ((TARGET vtkm::cuda) OR (TARGET vtkm::kokkos_cuda))
  # CUDA architecture has a limited amount of memory available for constants. The CUDA
  # compiler uses this space to hold constants for some optimizations. However, for large
  # kernels, the number of constants needed might be larger than the constant space
  # available. For these conditions, you have to disable this form of optimization with
  # the -Xptxas --disable-optimizer-constants flags.
  # TODO: Find a more elegant way to do this. Either figure out a way around this problem
  # or add more general flags to vtkm_library/vtkm_unit_tests for sources with "large" kernels.
  set_source_files_properties(lagrangian.cxx PROPERTIES
    COMPILE_OPTIONS "-Xptxas;--disable-optimizer-constants"
    )
endif()

add_executable(Lagrangian lagrangian.cxx ABCfield.h)
target_link_libraries(Lagrangian PRIVATE vtkm_filter)
vtkm_add_target_information(Lagrangian
                            DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES lagrangian.cxx)
