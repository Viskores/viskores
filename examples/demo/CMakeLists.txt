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
project(VTKmDemo CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

set(srcs Demo.cxx)

if(TARGET vtkm_rendering)
  set(backend "VTKM_DEVICE_ADAPTER_SERIAL")
  if(TARGET vtkm::cuda)
    vtkm_compile_as_cuda(cuda_srcs ${srcs})
    set(srcs ${cuda_srcs})
    set(backend "VTKM_DEVICE_ADAPTER_CUDA")
  elseif(TARGET vtkm::tbb)
    set(backend "VTKM_DEVICE_ADAPTER_TBB")
  endif()

  add_executable(Demo ${srcs})
  target_link_libraries(Demo PRIVATE vtkm_rendering)
  target_compile_definitions(Demo PRIVATE "VTKM_DEVICE_ADAPTER=${backend}")
endif()
