##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

#
function(vtkm_extract_real_library library real_library)
  if(NOT UNIX)
    set(${real_library} "${library}" PARENT_SCOPE)
    return()
  endif()

  #Read in the first 4 bytes and see if they are the ELF magic number
  set(_elf_magic "7f454c46")
  file(READ ${library} _hex_data OFFSET 0 LIMIT 4 HEX)
  if(_hex_data STREQUAL _elf_magic)
    #we have opened a elf binary so this is what
    #we should link too
    set(${real_library} "${library}" PARENT_SCOPE)
    return()
  endif()

  file(READ ${library} _data OFFSET 0 LIMIT 1024)
  if("${_data}" MATCHES "INPUT \\(([^(]+)\\)")
    #extract out the so name from REGEX MATCh command
    set(_proper_so_name "${CMAKE_MATCH_1}")

    #construct path to the real .so which is presumed to be in the same directory
    #as the input file
    get_filename_component(_so_dir "${library}" DIRECTORY)
    set(${real_library} "${_so_dir}/${_proper_so_name}" PARENT_SCOPE)
  else()
    #unable to determine what this library is so just hope everything works
    #add pass it unmodified.
    set(${real_library} "${library}" PARENT_SCOPE)
  endif()
endfunction()

if(VTKm_ENABLE_TBB AND NOT TARGET vtkm::tbb)
  find_package(TBB REQUIRED)

  # Workaround a bug in older versions of cmake prevents linking with UNKNOWN IMPORTED libraries
  # refer to CMake issue #17245
  if (CMAKE_VERSION VERSION_LESS 3.10)
    add_library(vtkm::tbb SHARED IMPORTED GLOBAL)
  else()
    add_library(vtkm::tbb UNKNOWN IMPORTED GLOBAL)
  endif()

  set_target_properties(vtkm::tbb PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${TBB_INCLUDE_DIRS}")

  if(EXISTS "${TBB_LIBRARY_RELEASE}")
    vtkm_extract_real_library("${TBB_LIBRARY_RELEASE}" real_path)
    set_property(TARGET vtkm::tbb APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(vtkm::tbb PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
      IMPORTED_LOCATION_RELEASE "${real_path}"
      )
  endif()

  if(EXISTS "${TBB_LIBRARY_DEBUG}")
    vtkm_extract_real_library("${TBB_LIBRARY_DEBUG}" real_path)
    set_property(TARGET vtkm::tbb APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
    set_target_properties(vtkm::tbb PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
      IMPORTED_LOCATION_DEBUG "${real_path}"
      )
  endif()
endif()


if(VTKm_ENABLE_OPENMP AND NOT TARGET vtkm::openmp)
  cmake_minimum_required(VERSION 3.9...3.12 FATAL_ERROR)
  find_package(OpenMP 4.0 REQUIRED COMPONENTS CXX QUIET)

  add_library(vtkm::openmp INTERFACE IMPORTED GLOBAL)
  if(OpenMP_CXX_FLAGS)
    set_property(TARGET vtkm::openmp
      APPEND PROPERTY INTERFACE_COMPILE_OPTIONS $<$<COMPILE_LANGUAGE:CXX>:${OpenMP_CXX_FLAGS}>)

    if(VTKm_ENABLE_CUDA)
      string(REPLACE ";" "," openmp_cuda_flags "-Xcompiler=${OpenMP_CXX_FLAGS}")
      set_property(TARGET vtkm::openmp
        APPEND PROPERTY INTERFACE_COMPILE_OPTIONS $<$<COMPILE_LANGUAGE:CUDA>:${openmp_cuda_flags}>)
    endif()
  endif()
  if(OpenMP_CXX_LIBRARIES)
    set_target_properties(vtkm::openmp PROPERTIES
      INTERFACE_LINK_LIBRARIES "${OpenMP_CXX_LIBRARIES}")
  endif()
endif()

if(VTKm_ENABLE_CUDA AND NOT TARGET vtkm::cuda)
  cmake_minimum_required(VERSION 3.9...3.12 FATAL_ERROR)
  enable_language(CUDA)

  #To work around https://gitlab.kitware.com/cmake/cmake/issues/17512
  #we need to fix the CMAKE_CUDA_IMPLICIT_INCLUDE_DIRECTORIES variable
  if(${CMAKE_VERSION} VERSION_LESS 3.10 AND CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
    list(APPEND CMAKE_CUDA_IMPLICIT_INCLUDE_DIRECTORIES "${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}")
  endif()

  # Workaround a bug in older versions of cmake prevents linking with UNKNOWN IMPORTED libraries
  # refer to CMake issue #17245
  if (CMAKE_VERSION VERSION_LESS 3.10)
    add_library(vtkm::cuda STATIC IMPORTED GLOBAL)
  else()
    add_library(vtkm::cuda UNKNOWN IMPORTED GLOBAL)
  endif()

  set_target_properties(vtkm::cuda PROPERTIES
    INTERFACE_COMPILE_OPTIONS $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>
  )

  # We can't have this location/lib empty, so we provide a location that is
  # valid and will have no effect on compilation
  if("x${CMAKE_CUDA_SIMULATE_ID}" STREQUAL "xMSVC")
    get_filename_component(VTKM_CUDA_BIN_DIR "${CMAKE_CUDA_COMPILER}" DIRECTORY)

    set_target_properties(vtkm::cuda PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${VTKM_CUDA_BIN_DIR}/../lib/x64/cudadevrt.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${VTKM_CUDA_BIN_DIR}/../include/"
        )

  else()
    list(GET CMAKE_CUDA_IMPLICIT_LINK_LIBRARIES 0 VTKM_CUDA_LIBRARY)
    if(IS_ABSOLUTE "${VTKM_CUDA_LIBRARY}")
      set_target_properties(vtkm::cuda PROPERTIES IMPORTED_LOCATION "${VTKM_CUDA_LIBRARY}")
    else()
      find_library(cuda_lib
              NAME ${VTKM_CUDA_LIBRARY}
              PATHS ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES}
              )
      set(VTKM_CUDA_LIBRARY ${cuda_lib})
      set_target_properties(vtkm::cuda PROPERTIES IMPORTED_LOCATION "${VTKM_CUDA_LIBRARY}")
      unset(cuda_lib CACHE)
    endif()
    set_target_properties(vtkm::cuda PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}")
  endif()

  # add the -gencode flags so that all cuda code
  # way compiled properly

  #---------------------------------------------------------------------------
  # Populates CMAKE_CUDA_FLAGS with the best set of flags to compile for a
  # given GPU architecture. The majority of developers should leave the
  # option at the default of 'native' which uses system introspection to
  # determine the smallest numerous of virtual and real architectures it
  # should target.
  #
  # The option of 'all' is provided for people generating libraries that
  # will deployed to any number of machines, it will compile all CUDA code
  # for all major virtual architectures, guaranteeing that the code will run
  # anywhere.
  #
  # The option 'none' is provided so that when being built as part of another
  # project, its own custom flags can be used.
  #
  # 1 - native
  #   - Uses system introspection to determine compile flags
  # 2 - fermi
  #   - Uses: --generate-code=arch=compute_20,code=sm_20
  # 3 - kepler
  #   - Uses: --generate-code=arch=compute_30,code=sm_30
  #   - Uses: --generate-code=arch=compute_35,code=sm_35
  # 4 - maxwell
  #   - Uses: --generate-code=arch=compute_50,code=sm_50
  # 5 - pascal
  #   - Uses: --generate-code=arch=compute_60,code=sm_60
  # 6 - volta
  #   - Uses: --generate-code=arch=compute_70,code=sm_70
  # 7 - all
  #   - Uses: --generate-code=arch=compute_30,code=sm_30
  #   - Uses: --generate-code=arch=compute_35,code=sm_35
  #   - Uses: --generate-code=arch=compute_50,code=sm_50
  #   - Uses: --generate-code=arch=compute_60,code=sm_60
  #   - Uses: --generate-code=arch=compute_70,code=sm_70
  # 8 - none
  #

  #specify the property
  set(VTKm_CUDA_Architecture "native" CACHE STRING "Which GPU Architecture(s) to compile for")
  set_property(CACHE VTKm_CUDA_Architecture PROPERTY STRINGS native fermi kepler maxwell pascal volta all none)

  #detect what the property is set too
  if(VTKm_CUDA_Architecture STREQUAL "native")

    if(VTKM_CUDA_NATIVE_EXE_PROCESS_RAN_OUTPUT)
      #Use the cached value
      set(arch_flags ${VTKM_CUDA_NATIVE_EXE_PROCESS_RAN_OUTPUT})
    else()

      #run execute_process to do auto_detection
      if(CMAKE_GENERATOR MATCHES "Visual Studio")
        set(args "-ccbin" "${CMAKE_CXX_COMPILER}" "--run" "${VTKm_CMAKE_MODULE_PATH}/VTKmDetectCUDAVersion.cu")
      elseif(CUDA_HOST_COMPILER)
        set(args "-ccbin" "${CUDA_HOST_COMPILER}" "--run" "${VTKm_CMAKE_MODULE_PATH}/VTKmDetectCUDAVersion.cu")
      else()
        set(args "--run" "${VTKm_CMAKE_MODULE_PATH}/VTKmDetectCUDAVersion.cu")
      endif()

      execute_process(
              COMMAND ${CMAKE_CUDA_COMPILER} ${args}
              RESULT_VARIABLE ran_properly
              OUTPUT_VARIABLE run_output
              WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

      if(ran_properly EQUAL 0)
        #find the position of the "--generate-code" output. With some compilers such as
        #msvc we get compile output plus run output. So we need to strip out just the
        #run output
        string(FIND "${run_output}" "--generate-code" position)
        string(SUBSTRING "${run_output}" ${position} -1 run_output)

        set(arch_flags ${run_output})
        set(VTKM_CUDA_NATIVE_EXE_PROCESS_RAN_OUTPUT ${run_output} CACHE INTERNAL
                "device type(s) for cuda[native]")
      else()
        set(VTKm_CUDA_Architecture "kepler")
      endif()
    endif()
  endif()

  #since when we are native we can fail, and fall back to "kepler" these have
  #to happen after, and separately of the native check
  if(VTKm_CUDA_Architecture STREQUAL "fermi")
    set(arch_flags --generate-code=arch=compute_20,code=sm_20)
  elseif(VTKm_CUDA_Architecture STREQUAL "kepler")
    set(arch_flags --generate-code=arch=compute_30,code=sm_30
                   --generate-code=arch=compute_35,code=sm_35)
  elseif(VTKm_CUDA_Architecture STREQUAL "maxwell")
    set(arch_flags --generate-code=arch=compute_50,code=sm_50)
  elseif(VTKm_CUDA_Architecture STREQUAL "pascal")
    set(arch_flags --generate-code=arch=compute_60,code=sm_60)
  elseif(VTKm_CUDA_Architecture STREQUAL "volta")
    set(arch_flags --generate-code=arch=compute_70,code=sm_70)
  elseif(VTKm_CUDA_Architecture STREQUAL "all")
    set(arch_flags --generate-code=arch=compute_30,code=sm_30
                   --generate-code=arch=compute_35,code=sm_35
                   --generate-code=arch=compute_50,code=sm_50
                   --generate-code=arch=compute_60,code=sm_60
                   --generate-code=arch=compute_70,code=sm_70)
  endif()

  string(REPLACE ";" " " arch_flags "${arch_flags}")
  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} ${arch_flags}")

  set_target_properties(vtkm::cuda PROPERTIES VTKm_CUDA_Architecture_Flags "${arch_flags}")

endif()
