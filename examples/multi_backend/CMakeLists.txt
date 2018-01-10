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
cmake_minimum_required(VERSION 3.3 FATAL_ERROR)
project(MultiBackend CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

set(srcs
    MultiBackend.cxx)

if(TARGET vtkm::cuda)
  vtkm_compile_as_cuda(cuda_srcs ${srcs})
  set(srcs ${cuda_srcs})
endif()

add_executable(MultiBackend ${srcs})
target_link_libraries(MultiBackend PRIVATE vtkm_cont)
