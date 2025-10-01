##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(PolyLineArchimedeanHelix CXX)

find_package(Viskores REQUIRED QUIET)

if (TARGET viskores::rendering)
  # TODO: This example should be changed from using the Tube worklet to using
  # the Tube filter (in the viskores::filter_geometry_refinement library). Then
  # compiling it would no longer require a device compiler and the example
  # would generally be simpler.
  add_executable(PolyLineArchimedeanHelix PolyLineArchimedeanHelix.cxx)
  target_link_libraries(PolyLineArchimedeanHelix PRIVATE viskores::rendering)
  viskores_add_target_information(PolyLineArchimedeanHelix
    DROP_UNUSED_SYMBOLS
    MODIFY_CUDA_FLAGS
    DEVICE_SOURCES PolyLineArchimedeanHelix.cxx
    )
endif()
