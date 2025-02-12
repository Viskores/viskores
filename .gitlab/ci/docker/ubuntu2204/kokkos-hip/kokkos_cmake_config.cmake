##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(CMAKE_BUILD_TYPE     "release"                  CACHE STRING "")
set(CMAKE_C_COMPILER     /opt/rocm/llvm/bin/clang   CACHE FILEPATH "")
set(CMAKE_CXX_COMPILER   /opt/rocm/llvm/bin/clang++ CACHE FILEPATH "")
set(CMAKE_POSITION_INDEPENDENT_CODE ON              CACHE BOOL "")

set(Kokkos_ENABLE_SERIAL ON                         CACHE BOOL "")
set(Kokkos_ENABLE_HIP    ON                         CACHE BOOL "")
set(Kokkos_ENABLE_HIP_RELOCATABLE_DEVICE_CODE OFF   CACHE BOOL "")
