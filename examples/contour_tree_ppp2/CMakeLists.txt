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

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET
  OPTIONAL_COMPONENTS Serial CUDA TBB
  )

# MAKE DEBUG BUILD
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -g -fsanitize=address")

# MAKE OPTIMIZED BUILD
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O3")

####################################
# Serial builds
####################################
# Serial 2D / 3D / MC
add_executable(ContourTree_PPP2_SERIAL ContourTreeApp.cxx)
target_link_libraries(ContourTree_PPP2_SERIAL vtkm_cont)
target_compile_definitions(ContourTree_PPP2_SERIAL PRIVATE
                           "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_SERIAL")


####################################
# Serial debug builds
####################################

# Debug Serial 2D / 3D / MC
add_executable(ContourTree_PPP2_SERIAL_DEBUG ContourTreeApp.cxx)
target_link_libraries(ContourTree_PPP2_SERIAL_DEBUG vtkm_cont)
target_compile_definitions(ContourTree_PPP2_SERIAL_DEBUG PRIVATE
                           "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_SERIAL" "DEBUG_PRINT")


if(TARGET vtkm::tbb)
  # TBB 2D/3D/MC
  add_executable(ContourTree_PPP2_TBB ContourTreeApp.cxx)
  target_link_libraries(ContourTree_PPP2_TBB vtkm_cont)
  target_compile_definitions(ContourTree_PPP2_TBB PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB" "ENABLE_SET_NUM_THREADS")

  # TBB 2D/3D/MC DEBUG
  add_executable(ContourTree_PPP2_TBB_DEBUG ContourTreeApp.cxx)
  target_link_libraries(ContourTree_PPP2_TBB_DEBUG vtkm_cont)
  target_compile_definitions(ContourTree_PPP2_TBB_DEBUG PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB" "ENABLE_SET_NUM_THREADS" "DEBUG_PRINT")

endif()

if(TARGET vtkm::cuda)
  # CUDA 2D/3D/MC
  vtkm_compile_as_cuda(cudaSource ContourTreeApp.cxx)
  add_executable(ContourTree_PPP2_CUDA ${cudaSource})
  target_link_libraries(ContourTree_PPP2_CUDA vtkm_cont)
  target_compile_definitions(ContourTree_PPP2_CUDA PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA")

  # CUDA 2D/3D/MC
  vtkm_compile_as_cuda(cudaSource ContourTreeApp.cxx)
  add_executable(ContourTree_PPP2_CUDA_DEBUG ${cudaSource})
  target_link_libraries(ContourTree_PPP2_CUDA_DEBUG vtkm_cont)
  target_compile_definitions(ContourTree_PPP2_CUDA_DEBUG PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA" "DEBUG_PRINT")
endif()
