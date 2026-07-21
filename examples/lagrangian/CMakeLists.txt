##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

#Find the Viskores package
find_package(Viskores REQUIRED QUIET)

if(TARGET viskores::filter_flow)
  add_executable(Lagrangian lagrangian.cxx ABCfield.h)
  target_link_libraries(Lagrangian PRIVATE viskores::filter_flow)
endif()
