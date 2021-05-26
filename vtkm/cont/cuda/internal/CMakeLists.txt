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
  CudaAllocator.h
  DeviceAdapterAlgorithmCuda.h
  DeviceAdapterMemoryManagerCuda.h
  DeviceAdapterRuntimeDetectorCuda.h
  DeviceAdapterTagCuda.h
  DeviceAdapterTimerImplementationCuda.h
  MakeThrustIterator.h
  RuntimeDeviceConfigurationCuda.h
  ScopedCudaStackSize.h
  ThrustExceptionHandler.h
  )

if (NOT VTKm_NO_DEPRECATED_VIRTUAL)
  set(headers ${headers}
    VirtualObjectTransferCuda.h
    )
endif()

vtkm_declare_headers(${headers})


if (TARGET vtkm::cuda)
  target_sources(vtkm_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/CudaAllocator.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmCuda.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterMemoryManagerCuda.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorCuda.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterTimerImplementationCuda.cu
    )
else()
#build the file with cpp compiler if cuda is disabled
  target_sources(vtkm_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorCuda.cxx
    )
endif()
