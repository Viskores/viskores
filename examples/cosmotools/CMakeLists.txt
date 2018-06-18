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
project(CosmoTools CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

add_executable(CosmoCenterFinder_SERIAL CosmoCenterFinder.cxx)
add_executable(CosmoHaloFinder_SERIAL CosmoHaloFinder.cxx)

target_link_libraries(CosmoCenterFinder_SERIAL PRIVATE vtkm_cont)
target_link_libraries(CosmoHaloFinder_SERIAL PRIVATE vtkm_cont)

if(TARGET vtkm::cuda)
  add_executable(CosmoCenterFinder_CUDA CosmoCenterFinder.cu)
  add_executable(CosmoHaloFinder_CUDA CosmoHaloFinder.cu)

  target_link_libraries(CosmoCenterFinder_CUDA PRIVATE vtkm_cont)
  target_link_libraries(CosmoHaloFinder_CUDA PRIVATE vtkm_cont)
endif()

if(TARGET vtkm::tbb)
  add_executable(CosmoCenterFinder_TBB CosmoCenterFinderTBB.cxx)
  add_executable(CosmoHaloFinder_TBB CosmoHaloFinderTBB.cxx)

  target_link_libraries(CosmoCenterFinder_TBB PRIVATE vtkm_cont)
  target_link_libraries(CosmoHaloFinder_TBB PRIVATE vtkm_cont)
endif()
