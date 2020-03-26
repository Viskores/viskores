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

string(REPLACE "+" ";" options "$ENV{VTKM_SETTINGS}")

foreach(option IN LISTS options)

  if(static STREQUAL option)
    set(BUILD_SHARED_LIBS "OFF" CACHE STRING "")

  elseif(shared STREQUAL option)
    set(BUILD_SHARED_LIBS "ON" CACHE STRING "")

  elseif(32bit_ids STREQUAL option)
    set(VTKm_USE_64BIT_IDS "OFF" CACHE STRING "")

  elseif(64bit_floats STREQUAL option)
    set(VTKm_USE_DOUBLE_PRECISION "ON" CACHE STRING "")

  elseif(mpi STREQUAL option)
    set(VTKm_ENABLE_MPI "ON" CACHE STRING "")

  elseif(tbb STREQUAL option)
    set(VTKm_ENABLE_TBB "ON" CACHE STRING "")

  elseif(openmp STREQUAL option)
    set(VTKm_ENABLE_OPENMP "ON" CACHE STRING "")

  elseif(cuda STREQUAL option)
    set(VTKm_ENABLE_CUDA "ON" CACHE STRING "")

  elseif(maxwell STREQUAL option)
    set(VTKm_CUDA_Architecture "maxwell" CACHE STRING "")

  elseif(pascal STREQUAL option)
    set(VTKm_CUDA_Architecture "pascal" CACHE STRING "")

  elseif(volta STREQUAL option)
    set(VTKm_CUDA_Architecture "volta" CACHE STRING "")

  elseif(turing STREQUAL option)
    set(VTKm_CUDA_Architecture "turing" CACHE STRING "")
  endif()

endforeach()

set(CTEST_USE_LAUNCHERS "ON" CACHE STRING "")
set(CMAKE_C_COMPILER_LAUNCHER "sccache" CACHE STRING "")
set(CMAKE_CXX_COMPILER_LAUNCHER "sccache" CACHE STRING "")
if(VTKm_ENABLE_CUDA)
  set(CMAKE_CUDA_COMPILER_LAUNCHER "sccache" CACHE STRING "")
endif()
