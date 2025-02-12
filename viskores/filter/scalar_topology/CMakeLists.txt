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
  SelectTopVolumeContoursFilter.h
  )

set(scalar_topology_sources
  internal/BranchDecompositionBlock.cxx
  internal/SelectTopVolumeContoursBlock.cxx
  internal/ComputeBlockIndices.cxx
  internal/ComputeDistributedBranchDecompositionFunctor.cxx
  internal/SelectTopVolumeContoursFunctor.cxx
  internal/ExchangeBranchEndsFunctor.cxx
  ContourTreeUniform.cxx
  ContourTreeUniformAugmented.cxx
  ContourTreeUniformDistributed.cxx
  DistributedBranchDecompositionFilter.cxx
  SelectTopVolumeContoursFilter.cxx
  )

viskores_library(
  NAME viskores_filter_scalar_topology
  HEADERS ${scalar_topology_headers}
  DEVICE_SOURCES ${scalar_topology_sources}
  USE_VISKORES_JOB_POOL
)

target_link_libraries(viskores_filter_scalar_topology PUBLIC viskores_worklet viskores_filter_core)
if (Viskores_ENABLE_MPI)
  target_link_libraries(viskores_filter_scalar_topology PUBLIC MPI::MPI_CXX)
endif ()
target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_scalar_topology)

add_subdirectory(internal)
add_subdirectory(worklet)
