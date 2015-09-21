##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2015 Sandia Corporation.
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================

cmake_minimum_required(VERSION 2.8.11)


add_executable(Clipping_SERIAL Clipping.cxx)
#target_include_directories(Clipping_SERIAL "")
#target_link_libraries(Clipping_SERIAL "")
set_property(
  TARGET Clipping_SERIAL
  APPEND
  PROPERTY COMPILE_DEFINITIONS "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_SERIAL")

if(VTKm_Cuda_FOUND)
  set (cudaSource "${CMAKE_CURRENT_BINARY_DIR}/Clipping.cu")
  configure_file(Clipping.cxx ${cudaSource} COPYONLY)

  vtkm_setup_nvcc_flags( old_nvcc_flags )
  vtkm_disable_troublesome_thrust_warnings()

  cuda_add_executable(Clipping_CUDA ${cudaSource})

  set(CUDA_NVCC_FLAGS ${old_nvcc_flags})

  #target_link_libraries(Clipping_CUDA "")
endif()

if(VTKm_ENABLE_TBB)
  add_executable(Clipping_TBB Clipping.cxx)
  target_include_directories(Clipping_TBB PRIVATE ${TBB_INCLUDE_DIRS})
  target_link_libraries(Clipping_TBB ${TBB_LIBRARIES})
  set_property(
    TARGET Clipping_TBB
    APPEND
    PROPERTY COMPILE_DEFINITIONS "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB")
endif()
