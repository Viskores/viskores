##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(HelloWorklet CXX)

#Find the Viskores package
find_package(Viskores REQUIRED QUIET)

if(TARGET viskores::io)
  add_executable(HelloWorklet HelloWorklet.cxx)
  target_link_libraries(HelloWorklet
    PRIVATE viskores::filter_core viskores::worklet viskores::io)

  viskores_add_target_information(HelloWorklet
    DROP_UNUSED_SYMBOLS
    MODIFY_CUDA_FLAGS
    DEVICE_SOURCES HelloWorklet.cxx)
endif()
