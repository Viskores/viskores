##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

set(headers
  ArrayManagerExecutionCuda.h
  CudaAllocator.h
  DeviceAdapterAlgorithmCuda.h
  DeviceAdapterAtomicArrayImplementationCuda.h
  DeviceAdapterRuntimeDetectorCuda.h
  DeviceAdapterTagCuda.h
  DeviceAdapterTimerImplementationCuda.h
  ExecutionArrayInterfaceBasicCuda.h
  MakeThrustIterator.h
  TaskTuner.h
  ThrustExceptionHandler.h
  VirtualObjectTransferCuda.h
  )

vtkm_declare_headers(CUDA ${headers} TESTABLE ${VTKm_ENABLE_CUDA})


if (NOT VTKm_ENABLE_CUDA)
  #build the file with cpp compiler if cuda is disabled
  target_sources(vtkm_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorCuda.cxx
    )

  return()
endif()

target_sources(vtkm_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/ArrayManagerExecutionCuda.cu
  ${CMAKE_CURRENT_SOURCE_DIR}/CudaAllocator.cu
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmCuda.cu
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorCuda.cu
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterTimerImplementationCuda.cu
  ${CMAKE_CURRENT_SOURCE_DIR}/ExecutionArrayInterfaceBasicCuda.cu
)
