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
  FilterParticleAdvection.h
  FilterParticleAdvectionSteadyState.h
  FilterParticleAdvectionUnsteadyState.h
  FlowTypes.h
  Lagrangian.h
  LagrangianStructures.h
  ParticleAdvection.h
  Pathline.h
  PathParticle.h
  Streamline.h
  StreamSurface.h
  WarpXStreamline.h
  )

set(flow_sources
  internal/Messenger.cxx
  FilterParticleAdvection.cxx
  )

set(flow_device_sources
  worklet/Analysis.cxx
  Lagrangian.cxx
  LagrangianStructures.cxx
  FilterParticleAdvectionSteadyState.cxx
  FilterParticleAdvectionUnsteadyState.cxx
  StreamSurface.cxx
  Lagrangian.cxx
  LagrangianStructures.cxx
  ParticleAdvection.cxx
  Pathline.cxx
  PathParticle.cxx
  Streamline.cxx
  WarpXStreamline.cxx
  )

vtkm_library(
  NAME vtkm_filter_flow
  HEADERS ${flow_headers}
  SOURCES ${flow_sources}
  DEVICE_SOURCES ${flow_device_sources}
  USE_VTKM_JOB_POOL
)

target_link_libraries(vtkm_filter PUBLIC INTERFACE vtkm_filter_flow)

add_subdirectory(internal)
add_subdirectory(worklet)
