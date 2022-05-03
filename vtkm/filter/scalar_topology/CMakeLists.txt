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
  BranchDecompositionBlock.h
  ComputeDistributedBranchDecompositionFunctor.h
  DistributedBranchDecompositionFilter.h
  )

set(scalar_topology_sources
  BranchDecompositionBlock.cxx
  ComputeDistributedBranchDecompositionFunctor.cxx
  DistributedBranchDecompositionFilter.cxx
  )

# TODO/FIXME: Is the following line needed
# set_source_files_properties(DistributedBranchDecompositionFilter.cxx PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

vtkm_library(
  NAME vtkm_filter_scalar_topology
  HEADERS ${scalar_topology_headers}
  DEVICE_SOURCES ${scalar_topology_sources}
  USE_VTKM_JOB_POOL
)

# TODO/FIXME: Are the following lines needed
# set_property(TARGET
#  vtkm_filter_scalar_topology
#  PROPERTY UNITY_BUILD_MODE GROUP
# )

target_link_libraries(vtkm_filter_scalar_topology PUBLIC vtkm_worklet vtkm_filter_core)
if (VTKm_ENABLE_MPI)
  target_link_libraries(vtkm_filter_scalar_topology PUBLIC MPI::MPI_CXX)
endif ()
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_scalar_topology)

add_subdirectory(worklet)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
