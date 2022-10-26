##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(scalar_topology_headers
  ContourTreeUniform.h
  ContourTreeUniformAugmented.h
  ContourTreeUniformDistributed.h
  DistributedBranchDecompositionFilter.h
  )

set(scalar_topology_sources
  internal/BranchDecompositionBlock.cxx
  internal/ComputeBlockIndices.cxx
  internal/ComputeDistributedBranchDecompositionFunctor.cxx
  ContourTreeUniform.cxx
  ContourTreeUniformAugmented.cxx
  ContourTreeUniformDistributed.cxx
  DistributedBranchDecompositionFilter.cxx
  )

vtkm_library(
  NAME vtkm_filter_scalar_topology
  HEADERS ${scalar_topology_headers}
  DEVICE_SOURCES ${scalar_topology_sources}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter_scalar_topology PUBLIC vtkm_worklet vtkm_filter_core)
if (VTKm_ENABLE_MPI)
  target_link_libraries(vtkm_filter_scalar_topology PUBLIC MPI::MPI_CXX)
endif ()
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_scalar_topology)

add_subdirectory(internal)
add_subdirectory(worklet)
