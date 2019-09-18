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
cmake_minimum_required(VERSION 3.8...3.15 FATAL_ERROR)
project(MeshQuality CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

add_executable(MeshQuality MeshQuality.cxx)
target_link_libraries(MeshQuality PRIVATE vtkm_filter)

if(TARGET vtkm::tbb)
  target_compile_definitions(MeshQuality PRIVATE BUILDING_TBB_VERSION)
endif()

vtkm_add_target_information(MeshQuality
                            DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES MeshQuality.cxx)
