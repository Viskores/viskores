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
project(ContourTree CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

add_executable(ContourTreeMesh2D_SERIAL ContourTreeMesh2D.cxx)
target_link_libraries(ContourTreeMesh2D_SERIAL vtkm_cont)

add_executable(ContourTreeMesh3D_SERIAL ContourTreeMesh3D.cxx)
target_link_libraries(ContourTreeMesh3D_SERIAL vtkm_cont)

target_compile_definitions(ContourTreeMesh2D_SERIAL PRIVATE
                           "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_SERIAL")
target_compile_definitions(ContourTreeMesh3D_SERIAL PRIVATE
                           "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_SERIAL")

if(TARGET vtkm::cuda)
  vtkm_compile_as_cuda(cudaSource ContourTreeMesh2D.cxx)
  add_executable(ContourTreeMesh2D_CUDA ${cudaSource})

  vtkm_compile_as_cuda(cudaSource ContourTreeMesh3D.cxx)
  add_executable(ContourTreeMesh3D_CUDA ${cudaSource})

  target_link_libraries(ContourTreeMesh2D_CUDA vtkm_cont)
  target_link_libraries(ContourTreeMesh3D_CUDA vtkm_cont)

  target_compile_definitions(ContourTreeMesh2D_CUDA PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA")
  target_compile_definitions(ContourTreeMesh3D_CUDA PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA")
endif()

if(TARGET vtkm::tbb)
  add_executable(ContourTreeMesh2D_TBB ContourTreeMesh2D.cxx)
  add_executable(ContourTreeMesh3D_TBB ContourTreeMesh3D.cxx)

  target_link_libraries(ContourTreeMesh2D_TBB vtkm_cont)
  target_link_libraries(ContourTreeMesh3D_TBB vtkm_cont)

  target_compile_definitions(ContourTreeMesh2D_TBB PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB")
  target_compile_definitions(ContourTreeMesh3D_TBB PRIVATE
                             "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB")
endif()
