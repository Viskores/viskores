##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(flow_headers
  AdvectAlgorithm.h
  AdvectAlgorithmThreaded.h
  BoundsMap.h
  DataSetIntegrator.h
  DataSetIntegratorSteadyState.h
  DataSetIntegratorUnsteadyState.h
  Messenger.h
  NewFilterParticleAdvection.h
  NewFilterParticleAdvectionSteadyState.h
  NewFilterParticleAdvectionUnsteadyState.h
  FlowTypes.h
  ParticleAdvector.h
  ParticleMessenger.h
  ParticleAdvection.h
  Pathline.h
  PathParticle.h
  Streamline.h
  )

set(flow_sources
  Messenger.cxx
  ParticleAdvection.cxx
  Pathline.cxx
  PathParticle.cxx
  Streamline.cxx
  )

vtkm_library(
  NAME vtkm_filter_flow
  HEADERS ${flow_headers}
  DEVICE_SOURCES ${flow_sources}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter_flow PUBLIC vtkm_worklet vtkm_filter_core)
target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_flow)
if (VTKm_ENABLE_MPI)
  target_link_libraries(vtkm_filter_flow PUBLIC MPI::MPI_CXX)
endif()

add_subdirectory(worklet)

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()
