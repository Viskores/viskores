##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION 3.8...3.14 FATAL_ERROR)
project(PolyLineArchimedeanHelix CXX)

find_package(VTKm REQUIRED QUIET)
add_executable(PolyLineArchimedeanHelix PolyLineArchimedeanHelix.cxx)
if(TARGET vtkm::cuda)
    set_source_files_properties(PolyLineArchimedeanHelix.cxx PROPERTIES LANGUAGE "CUDA")
endif()
target_link_libraries(PolyLineArchimedeanHelix PRIVATE vtkm_filter vtkm_cont vtkm_rendering)
