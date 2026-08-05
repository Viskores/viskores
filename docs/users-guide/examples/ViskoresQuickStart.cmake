##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


####
#### BEGIN-EXAMPLE QuickStartCMakeLists.txt
####
cmake_minimum_required(VERSION 3.15)
project(ViskoresQuickStart CXX)

find_package(Viskores REQUIRED)

add_executable(ViskoresQuickStart ViskoresQuickStart.cxx)
target_link_libraries(ViskoresQuickStart viskores::filter viskores::rendering)
####
#### END-EXAMPLE QuickStartCMakeLists.txt
####
