##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(ViskoresDemo CXX)

#Find the Viskores package
find_package(Viskores REQUIRED QUIET)

if(TARGET viskores::rendering AND TARGET viskores::filter_contour AND TARGET viskores::source)
  add_executable(Demo Demo.cxx)
  target_link_libraries(Demo PRIVATE viskores::rendering viskores::filter_contour viskores::source)
endif()
