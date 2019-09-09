##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION 3.8...3.15 FATAL_ERROR)
project(PolyLineArchimedeanHelix CXX)

find_package(VTKm REQUIRED QUIET)
if (VTKm_ENABLE_RENDERING)
    add_executable(PolyLineArchimedeanHelix PolyLineArchimedeanHelix.cxx)
    vtkm_add_target_information(PolyLineArchimedeanHelix
                                DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                                DEVICE_SOURCES PolyLineArchimedeanHelix.cxx)
    target_link_libraries(PolyLineArchimedeanHelix PRIVATE vtkm_filter vtkm_rendering)
endif()
