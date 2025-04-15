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
cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(ContourTree CXX)

#Find the Viskores package
find_package(Viskores REQUIRED QUIET)

if (TARGET viskores::filter_scalar_topology)
  add_executable(ContourTreeMesh2D ContourTreeMesh2D.cxx)
  target_link_libraries(ContourTreeMesh2D viskores::filter_core viskores::filter_scalar_topology)

  add_executable(ContourTreeMesh3D ContourTreeMesh3D.cxx)
  target_link_libraries(ContourTreeMesh3D viskores::filter_core viskores::filter_scalar_topology)
endif()
