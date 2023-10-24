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

  elseif(ascent_types STREQUAL option)
    # Note: ascent_types also requires 32bit_ids and 64bit_floats
    set(VTKm_USE_DEFAULT_TYPES_FOR_ASCENT "ON" CACHE STRING "")

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

  elseif(anari STREQUAL option)
    set(VTKm_ENABLE_ANARI "ON" CACHE STRING "")

  elseif(no_testing STREQUAL option)
    set(VTKm_ENABLE_TESTING "OFF" CACHE STRING "")
    set(VTKm_ENABLE_TESTING_LIBRARY "OFF" CACHE STRING "")

  elseif(examples STREQUAL option)
    set(VTKm_ENABLE_EXAMPLES "ON" CACHE STRING "")
    set(VTKm_INSTALL_EXAMPLES "ON" CACHE STRING "")

  elseif(docs STREQUAL option)
    set(VTKm_ENABLE_DOCUMENTATION "ON" CACHE STRING "")
    set(VTKm_USERS_GUIDE_INCLUDE_TODOS "OFF" CACHE STRING "")

  elseif(benchmarks STREQUAL option)
    set(VTKm_ENABLE_BENCHMARKS "ON" CACHE STRING "")
    set(ENV{CMAKE_PREFIX_PATH} "$ENV{CMAKE_PREFIX_PATH}:$ENV{HOME}/gbench")

  elseif(min_build STREQUAL option)
    set(VTKm_BUILD_ALL_LIBRARIES "OFF" CACHE STRING "")
    set(VTKm_VERBOSE_MODULES "ON" CACHE STRING "")

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

  elseif(hdf5 STREQUAL option)
    set(VTKm_ENABLE_HDF5_IO "ON" CACHE STRING "")

  elseif(maxwell STREQUAL option)
    set(vtkm_cuda_arch "maxwell")

  elseif(pascal STREQUAL option)
    set(vtkm_cuda_arch "pascal")

  elseif(volta STREQUAL option)
    set(vtkm_cuda_arch "volta")

  elseif(turing STREQUAL option)
    set(vtkm_cuda_arch "turing")

  elseif(ampere STREQUAL option)
    set(vtkm_cuda_arch "ampere")

  elseif(hip STREQUAL option)
    if(CMAKE_VERSION VERSION_LESS_EQUAL 3.20)
      message(FATAL_ERROR "VTK-m requires cmake > 3.20 to enable HIP support")
    endif()

    set(VTKm_ENABLE_KOKKOS_HIP ON CACHE STRING "")

    # -O1 and -O2 results in ridiculous build times in ROCm < 5.3
    set(CMAKE_HIP_FLAGS "-O0 " CACHE STRING "")
    set(CMAKE_HIP_FLAGS_RELWITHDEBINFO "-g" CACHE STRING "")

  elseif(ascent STREQUAL option)
    set(CMAKE_C_FLAGS "-mcpu=power9" CACHE STRING "")
    set(CMAKE_CXX_FLAGS "-mcpu=power9" CACHE STRING "")

  elseif(ccache STREQUAL option)
    find_program(CCACHE_COMMAND NAMES ccache REQUIRED)

    set(CCACHE_VERSION "NotFound")
    execute_process(
      COMMAND ${CCACHE_COMMAND} "--version"
      OUTPUT_VARIABLE CCACHE_VERSION
      ECHO_ERROR_VARIABLE
      )

    string(REGEX REPLACE "\n" " " CCACHE_VERSION ${CCACHE_VERSION})
    string(REGEX REPLACE "^.*ccache version ([.0-9]*).*$" "\\1"
      CCACHE_VERSION ${CCACHE_VERSION})

    # We need a recent version of ccache in order to ignore -isystem while
    # hashing keys for the building cache.
    if(${CCACHE_VERSION} VERSION_GREATER_EQUAL 4)
      set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_COMMAND}" CACHE STRING "")
      set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_COMMAND}" CACHE STRING "")

      if(VTKm_ENABLE_CUDA)
        set(CMAKE_CUDA_COMPILER_LAUNCHER "${CCACHE_COMMAND}" CACHE STRING "")
      endif()
      if(VTKm_ENABLE_KOKKOS_HIP)
        set(CMAKE_HIP_COMPILER_LAUNCHER "${CCACHE_COMMAND}" CACHE STRING "")
      endif()
    else()
      message(FATAL_ERROR "CCACHE version [${CCACHE_VERSION}] is <= 4")
    endif()

  elseif(perftest STREQUAL option)
    set(VTKm_ENABLE_PERFORMANCE_TESTING "ON" CACHE STRING "")
  endif()

endforeach()

# We need to use VTKm_CUDA_Architecture for older CMake versions
if(vtkm_cuda_arch)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
    if(vtkm_cuda_arch STREQUAL "maxwell")
      set(CMAKE_CUDA_ARCHITECTURES "50" CACHE STRING "")
    elseif(vtkm_cuda_arch STREQUAL "pascal")
      set(CMAKE_CUDA_ARCHITECTURES "60" CACHE STRING "")
    elseif(vtkm_cuda_arch STREQUAL "volta")
      set(CMAKE_CUDA_ARCHITECTURES "70" CACHE STRING "")
    elseif(vtkm_cuda_arch STREQUAL "turing")
      set(CMAKE_CUDA_ARCHITECTURES "75" CACHE STRING "")
    elseif(vtkm_cuda_arch STREQUAL "ampere")
      set(CMAKE_CUDA_ARCHITECTURES "80" CACHE STRING "")
    endif()
  else()
    set(VTKm_CUDA_Architecture "${vtkm_cuda_arch}" CACHE STRING "")
  endif()
endif()

# Compile tutorials on all builders. The code is small and basic. And since
# it is the tutorial, it should work really well.
set(VTKm_ENABLE_TUTORIALS "ON" CACHE STRING "")

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
  if(DEFINED VTKm_CUDA_Architecture OR DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_COMPILER_LAUNCHER "${SCCACHE_COMMAND}" CACHE STRING "")
  endif()
endif()

# Setup all the sanitizers as a list
if(sanitizers)
  set(VTKm_USE_SANITIZER "${sanitizers}"  CACHE STRING "" FORCE)
endif()
