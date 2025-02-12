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
project(StreamlineMPI CXX)

#Find the Viskores package
find_package(Viskores REQUIRED QUIET)

if (Viskores_ENABLE_MPI AND TARGET viskores::io AND TARGET viskores::filter_flow)
  add_executable(StreamlineMPI StreamlineMPI.cxx)
  target_compile_definitions(StreamlineMPI PRIVATE "MPI_ENABLED")
  target_link_libraries(StreamlineMPI PRIVATE viskores::filter_flow viskores::io MPI::MPI_CXX)
endif()
