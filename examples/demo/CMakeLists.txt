##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION 3.8...3.14 FATAL_ERROR)
project(VTKmDemo CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

set(srcs Demo.cxx)

if(TARGET vtkm_rendering)

  if(TARGET vtkm::cuda)
    vtkm_compile_as_cuda(cuda_srcs ${srcs})
    set(srcs ${cuda_srcs})
  endif()

  add_executable(Demo ${srcs})
  target_link_libraries(Demo PRIVATE vtkm_rendering)
endif()
