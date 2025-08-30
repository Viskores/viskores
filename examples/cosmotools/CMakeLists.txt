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
project(CosmoTools CXX)

#Find the Viskores package
find_package(Viskores REQUIRED QUIET)

add_executable(CosmoCenterFinder CosmoCenterFinder.cxx)
add_executable(CosmoHaloFinder CosmoHaloFinder.cxx)
target_link_libraries(CosmoCenterFinder PRIVATE viskores::filter_core viskores::worklet)
target_link_libraries(CosmoHaloFinder PRIVATE viskores::filter_core viskores::worklet)

viskores_add_target_information(CosmoCenterFinder CosmoHaloFinder
                            DROP_UNUSED_SYMBOLS
                            MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES
                              CosmoCenterFinder.cxx CosmoHaloFinder.cxx)
