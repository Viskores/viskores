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

# Default to Release builds.
if ("$ENV{CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")
else ()
  set(CMAKE_BUILD_TYPE "$ENV{CMAKE_BUILD_TYPE}" CACHE STRING "")
endif ()

string(REPLACE "+" ";" options "$ENV{VTKM_SETTINGS}")

foreach(option IN LISTS options)
  if(static STREQUAL option)
    set(BUILD_SHARED_LIBS "OFF" CACHE STRING "")

  elseif(shared STREQUAL option)
    set(BUILD_SHARED_LIBS "ON" CACHE STRING "")

  elseif(vtk_types STREQUAL option)
    set(VTKm_USE_DEFAULT_TYPES_FOR_VTK "ON" CACHE STRING "")

  elseif(32bit_ids STREQUAL option)
    set(VTKm_USE_64BIT_IDS "OFF" CACHE STRING "")

  elseif(64bit_floats STREQUAL option)
    set(VTKm_USE_DOUBLE_PRECISION "ON" CACHE STRING "")

  elseif(asan STREQUAL option)
    set(VTKm_ENABLE_SANITIZER "ON" CACHE STRING "")
    list(APPEND sanitizers "address")

  elseif(leak STREQUAL option)
    set(VTKm_ENABLE_SANITIZER "ON" CACHE STRING "")
    list(APPEND sanitizers "leak")

  elseif(rendering STREQUAL option)
    set(VTKm_ENABLE_RENDERING "ON" CACHE STRING "")

  elseif(no_rendering STREQUAL option)
    set(VTKm_ENABLE_RENDERING "OFF" CACHE STRING "")

  elseif(no_virtual STREQUAL option)
    set(VTKm_NO_DEPRECATED_VIRTUAL "ON" CACHE STRING "")

  elseif(examples STREQUAL option)
    set(VTKm_ENABLE_EXAMPLES "ON" CACHE STRING "")

  elseif(docs STREQUAL option)
    set(VTKm_ENABLE_DOCUMENTATION "ON" CACHE STRING "")

  elseif(benchmarks STREQUAL option)
    set(VTKm_ENABLE_BENCHMARKS "ON" CACHE STRING "")

  elseif(mpi STREQUAL option)
    set(VTKm_ENABLE_MPI "ON" CACHE STRING "")

  elseif(tbb STREQUAL option)
    set(VTKm_ENABLE_TBB "ON" CACHE STRING "")

  elseif(openmp STREQUAL option)
    set(VTKm_ENABLE_OPENMP "ON" CACHE STRING "")

  elseif(cuda STREQUAL option)
    set(VTKm_ENABLE_CUDA "ON" CACHE STRING "")

  elseif(kokkos STREQUAL option)
    set(VTKm_ENABLE_KOKKOS "ON" CACHE STRING "")

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

# We need to store the absolute path so that
# the launcher still work even when sccache isn't
# on our path.
find_program(SCCACHE_COMMAND NAMES sccache)
if(SCCACHE_COMMAND)
  set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_COMMAND}" CACHE STRING "")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_COMMAND}" CACHE STRING "")

  # Use VTKm_CUDA_Architecture to determine if we need CUDA sccache setup
  # since this will also capture when kokkos is being used with CUDA backing
  if(DEFINED VTKm_CUDA_Architecture)
    set(CMAKE_CUDA_COMPILER_LAUNCHER "${SCCACHE_COMMAND}" CACHE STRING "")
  endif()
endif()

# Setup all the sanitizers as a list
if(sanitizers)
  set(VTKm_USE_SANITIZER "${sanitizers}"  CACHE STRING "" FORCE)
endif()
