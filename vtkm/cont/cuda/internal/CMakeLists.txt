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
  ArrayManagerExecutionCuda.h
  AtomicInterfaceExecutionCuda.h
  CudaAllocator.h
  DeviceAdapterAlgorithmCuda.h
  DeviceAdapterRuntimeDetectorCuda.h
  DeviceAdapterTagCuda.h
  DeviceAdapterTimerImplementationCuda.h
  ExecutionArrayInterfaceBasicCuda.h
  MakeThrustIterator.h
  ThrustExceptionHandler.h
  VirtualObjectTransferCuda.h
  )

vtkm_declare_headers(${headers})


if (TARGET vtkm::cuda)
  target_sources(vtkm_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ArrayManagerExecutionCuda.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/CudaAllocator.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmCuda.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorCuda.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterTimerImplementationCuda.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/ExecutionArrayInterfaceBasicCuda.cu
    )
else()
#build the file with cpp compiler if cuda is disabled
  target_sources(vtkm_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorCuda.cxx
    )
endif()
