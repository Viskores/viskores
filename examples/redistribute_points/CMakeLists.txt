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
project(RedistributePoints CXX)

#Find the Viskores package
find_package(Viskores REQUIRED QUIET)

if(TARGET viskores::io AND TARGET viskores::filter_entity_extraction)
  add_executable(RedistributePoints RedistributePoints.cxx RedistributePoints.h main.cxx)
  target_link_libraries(RedistributePoints PRIVATE viskores::io viskores::filter_entity_extraction viskores::worklet)
  viskores_add_target_information(RedistributePoints
    DROP_UNUSED_SYMBOLS
    MODIFY_CUDA_FLAGS
    DEVICE_SOURCES RedistributePoints.cxx
    )

  set_property(TARGET RedistributePoints PROPERTY CUDA_VISIBILITY_PRESET "hidden")
  set_property(TARGET RedistributePoints PROPERTY CXX_VISIBILITY_PRESET "hidden")
endif()
