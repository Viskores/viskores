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
  ArrayManagerExecutionThrustDevice.h
  CudaAllocator.h
  DeviceAdapterAlgorithmCuda.h
  DeviceAdapterAlgorithmThrust.h
  DeviceAdapterTagCuda.h
  MakeThrustIterator.h
  TaskTuner.h
  ThrustExceptionHandler.h
  VirtualObjectTransferCuda.h
  )
vtkm_declare_headers(CUDA ${headers} TESTABLE ${VTKm_ENABLE_CUDA})

#-----------------------------------------------------------------------------
if (NOT VTKm_ENABLE_CUDA)
  return()
endif()

add_library(vtkm_cont_cuda OBJECT
  ArrayManagerExecutionCuda.cu
  CudaAllocator.cu
  )

target_compile_options(vtkm_cont_cuda PRIVATE $<TARGET_PROPERTY:vtkm_cont,INTERFACE_COMPILE_OPTIONS>)
target_compile_definitions(vtkm_cont_cuda PRIVATE $<TARGET_PROPERTY:vtkm_cont,INTERFACE_COMPILE_DEFINITIONS>)
target_compile_features(vtkm_cont_cuda PRIVATE $<TARGET_PROPERTY:vtkm_cont,INTERFACE_COMPILE_FEATURES>)
target_include_directories(vtkm_cont_cuda PRIVATE $<TARGET_PROPERTY:vtkm_cont,INTERFACE_INCLUDE_DIRECTORIES>)

if(BUILD_SHARED_LIBS)
  set_property(TARGET vtkm_cont_cuda PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()
