##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


#Find the Viskores package
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
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
