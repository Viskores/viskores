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
project(Clipping CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

add_executable(Clipping_SERIAL Clipping.cxx)
target_compile_definitions(Clipping_SERIAL PRIVATE
                           "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_SERIAL")
target_link_libraries(Clipping_SERIAL PRIVATE vtkm_cont)

if(TARGET vtkm::tbb)
  add_executable(Clipping_TBB Clipping.cxx)
  target_compile_definitions(Clipping_TBB PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB")
  target_link_libraries(Clipping_TBB PRIVATE vtkm_cont)
endif()

if(TARGET vtkm::cuda)
  vtkm_compile_as_cuda(cudaSource Clipping.cxx)
  add_executable(Clipping_CUDA ${cudaSource})

  target_compile_definitions(Clipping_CUDA PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA")
  target_link_libraries(Clipping_CUDA PRIVATE vtkm_cont)
endif()
