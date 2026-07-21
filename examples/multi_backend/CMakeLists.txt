##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(MultiBackend CXX)

#Find the Viskores package
find_package(Viskores REQUIRED)
find_package(Threads REQUIRED QUIET)

set(headers
  IOGenerator.h
  MultiDeviceGradient.h
  TaskQueue.h
  )

set(srcs
  MultiBackend.cxx
  MultiDeviceGradient.cxx
  IOGenerator.cxx
  )

if(TARGET viskores::filter_vector_analysis)
  add_executable(MultiBackend ${srcs} ${headers})
  target_link_libraries(MultiBackend PRIVATE viskores::filter_vector_analysis viskores::worklet Threads::Threads)
  viskores_add_target_information(MultiBackend
    DROP_UNUSED_SYMBOLS
    MODIFY_CUDA_FLAGS
    DEVICE_SOURCES ${srcs})
endif()
