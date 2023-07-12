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
project(VTKmQuickStart CXX)

find_package(VTKm REQUIRED)

add_executable(VTKmQuickStart VTKmQuickStart.cxx)
target_link_libraries(VTKmQuickStart vtkm::filter vtkm::rendering)
####
#### END-EXAMPLE QuickStartCMakeLists.txt
####
