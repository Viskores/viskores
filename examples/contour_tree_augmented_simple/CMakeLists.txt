##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(ContourTreeAugmentedSimple CXX)

find_package(Viskores REQUIRED QUIET)

if (TARGET viskores::filter_scalar_topology)
  add_executable(ContourTreeAugmentedSimple ContourTreeSimple.cxx)
  target_link_libraries(ContourTreeAugmentedSimple PRIVATE
    viskores::filter_scalar_topology
    viskores::worklet)
  viskores_add_target_information(ContourTreeAugmentedSimple
    DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
    DEVICE_SOURCES ContourTreeSimple.cxx)
endif()
