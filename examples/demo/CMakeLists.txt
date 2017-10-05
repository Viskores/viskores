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
##  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2015 UT-Battelle, LLC.
##  Copyright 2015 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(VTKmDemo CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

if(TARGET vtkm_rendering)

  add_executable(Demo Demo.cxx)
  target_link_libraries(Demo PRIVATE vtkm_rendering)

  if(TARGET vtkm::cuda)
    set_source_files_properties(Demo.cxx LANGUAGE "CUDA")
    target_compile_definitions(Demo
        PRIVATE "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA")
  elseif(TARGET vtkm::tbb)
    target_compile_definitions(Demo
        PRIVATE "VTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_TBB")
  endif()
endif()
