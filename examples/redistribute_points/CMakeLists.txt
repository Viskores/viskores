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
##  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
cmake_minimum_required(VERSION 3.3...3.12 FATAL_ERROR)
project(RedistributePoints CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
add_executable(RedistributePoints_SERIAL RedistributePoints.cxx RedistributePoints.h)
target_compile_definitions(RedistributePoints_SERIAL PRIVATE
                           "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_SERIAL")
target_link_libraries(RedistributePoints_SERIAL PRIVATE vtkm_cont)

if(TARGET vtkm::tbb)
  add_executable(RedistributePoints_TBB RedistributePoints.cxx RedistributePoints.h)
  target_compile_definitions(RedistributePoints_TBB PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB")
  target_link_libraries(RedistributePoints_TBB PRIVATE vtkm_cont)
endif()

if(TARGET vtkm::cuda)
  vtkm_compile_as_cuda(cudaSource RedistributePoints.cxx)
  add_executable(RedistributePoints_CUDA ${cudaSource} RedistributePoints.h)

  target_compile_definitions(RedistributePoints_CUDA PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA")
  target_link_libraries(RedistributePoints_CUDA PRIVATE vtkm_cont)
endif()
