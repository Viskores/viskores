##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#Find the Viskores package
cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(TemporalAdvection CXX)

#Find the Viskores package
find_package(Viskores REQUIRED QUIET)

if(TARGET viskores::filter_flow AND TARGET viskores::io)
  add_executable(Temporal_Advection TemporalAdvection.cxx)
  viskores_add_target_information(Temporal_Advection
    DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
    DEVICE_SOURCES TemporalAdvection.cxx)
  target_link_libraries(Temporal_Advection PRIVATE viskores::filter_flow viskores::io)
endif()
