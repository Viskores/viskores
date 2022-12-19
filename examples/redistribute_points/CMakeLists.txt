##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(RedistributePoints CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if(TARGET vtkm::io AND TARGET vtkm::filter_entity_extraction)
  add_executable(RedistributePoints RedistributePoints.cxx RedistributePoints.h main.cxx)
  target_link_libraries(RedistributePoints PRIVATE vtkm::io vtkm::filter_entity_extraction)
  vtkm_add_target_information(RedistributePoints
    DROP_UNUSED_SYMBOLS
    MODIFY_CUDA_FLAGS
    DEVICE_SOURCES RedistributePoints.cxx
    )

  set_property(TARGET RedistributePoints PROPERTY CUDA_VISIBILITY_PRESET "hidden")
  set_property(TARGET RedistributePoints PROPERTY CXX_VISIBILITY_PRESET "hidden")
endif()
