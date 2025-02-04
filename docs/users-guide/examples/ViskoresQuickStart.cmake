##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##=============================================================================

####
#### BEGIN-EXAMPLE QuickStartCMakeLists.txt
####
cmake_minimum_required(VERSION 3.13)
project(ViskoresQuickStart CXX)

find_package(Viskores REQUIRED)

add_executable(ViskoresQuickStart ViskoresQuickStart.cxx)
target_link_libraries(ViskoresQuickStart viskores::filter viskores::rendering)
####
#### END-EXAMPLE QuickStartCMakeLists.txt
####
