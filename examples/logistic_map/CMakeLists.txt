##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

cmake_minimum_required(VERSION 3.15...3.19 FATAL_ERROR)
project(LogisticMap CXX)

find_package(Viskores REQUIRED QUIET)

if(TARGET viskores::io)
  add_executable(LogisticMap LogisticMap.cxx)
  target_link_libraries(LogisticMap PRIVATE viskores::io viskores::worklet)

  viskores_add_target_information(LogisticMap
    DROP_UNUSED_SYMBOLS
    MODIFY_CUDA_FLAGS
    DEVICE_SOURCES LogisticMap.cxx)
endif()
