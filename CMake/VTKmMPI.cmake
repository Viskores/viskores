##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

if(VTKm_ENABLE_MPI AND NOT TARGET MPI::MPI_CXX)
  if(CMAKE_VERSION VERSION_LESS 3.15)
    #While CMake 3.10 introduced the new MPI module.
    #Fixes related to MPI+CUDA that VTK-m needs are
    #only found in CMake 3.15+.
    find_package(MPI REQUIRED MODULE)
  else()
    #clunky but we need to make sure we use the upstream module if it exists
    set(orig_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
    set(CMAKE_MODULE_PATH "")
    find_package(MPI REQUIRED MODULE)
    set(CMAKE_MODULE_PATH ${orig_CMAKE_MODULE_PATH})
  endif()
endif()
