##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
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
  elseif(EXISTS "${TBB_LIBRARY}")
    #When VTK-m is mixed with OSPray we could use the OSPray FindTBB file
    #which doesn't define TBB_LIBRARY_RELEASE but instead defined only
    #TBB_LIBRARY
    vtkm_extract_real_library("${TBB_LIBRARY}" real_path)
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
  cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
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

if(VTKm_ENABLE_CUDA)
  cmake_minimum_required(VERSION 3.13...3.15 FATAL_ERROR)
  enable_language(CUDA)

  if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA" AND
    CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 9.2)
    message(FATAL_ERROR "VTK-m CUDA support requires version 9.2+")
  endif()

 if (NOT TARGET vtkm::cuda)
    add_library(vtkm_cuda INTERFACE)
    add_library(vtkm::cuda ALIAS vtkm_cuda)
    set_target_properties(vtkm_cuda PROPERTIES EXPORT_NAME vtkm::cuda)

    install(TARGETS vtkm_cuda EXPORT ${VTKm_EXPORT_NAME})
    # Reserve `requires_static_builds` to potential work around issues
    # where VTK-m doesn't work when building shared as virtual functions fail
    # inside device code. We don't want to force BUILD_SHARED_LIBS to a specific
    # value as that could impact other projects that embed VTK-m. Instead what
    # we do is make sure that libraries built by vtkm_library() are static
    # if they use CUDA
    #
    # This needs to be lower-case for the property to be properly exported
    # CMake 3.15 we can add `requires_static_builds` to the EXPORT_PROPERTIES
    # target property to have this automatically exported for us
    set_target_properties(vtkm_cuda PROPERTIES
      requires_static_builds TRUE
    )


    set_target_properties(vtkm_cuda PROPERTIES
      INTERFACE_COMPILE_OPTIONS $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>
    )

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
    # 7 - turing
    #   - Uses: --generate-code=arch=compute_75code=sm_75
    # 8 - all
    #   - Uses: --generate-code=arch=compute_30,code=sm_30
    #   - Uses: --generate-code=arch=compute_35,code=sm_35
    #   - Uses: --generate-code=arch=compute_50,code=sm_50
    #   - Uses: --generate-code=arch=compute_60,code=sm_60
    #   - Uses: --generate-code=arch=compute_70,code=sm_70
    #   - Uses: --generate-code=arch=compute_75,code=sm_75
    # 8 - none
    #

    #specify the property
    set(VTKm_CUDA_Architecture "native" CACHE STRING "Which GPU Architecture(s) to compile for")
    set_property(CACHE VTKm_CUDA_Architecture PROPERTY STRINGS native fermi kepler maxwell pascal volta turing all none)

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
          message(FATAL_ERROR "Error detecting architecture flags for CUDA. Please set VTKm_CUDA_Architecture manually.")
        endif()
      endif()
    endif()

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
    elseif(VTKm_CUDA_Architecture STREQUAL "turing")
      set(arch_flags --generate-code=arch=compute_75,code=sm_75)
    elseif(VTKm_CUDA_Architecture STREQUAL "all")
      set(arch_flags --generate-code=arch=compute_30,code=sm_30
                     --generate-code=arch=compute_35,code=sm_35
                     --generate-code=arch=compute_50,code=sm_50
                     --generate-code=arch=compute_60,code=sm_60
                     --generate-code=arch=compute_70,code=sm_70
                     --generate-code=arch=compute_75,code=sm_75)
    endif()

    string(REPLACE ";" " " arch_flags "${arch_flags}")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} ${arch_flags}")

    # This needs to be lower-case for the property to be properly exported
    # CMake 3.15 we can add `cuda_architecture_flags` to the EXPORT_PROPERTIES
    # target property to have this automatically exported for us
    set_target_properties(vtkm_cuda PROPERTIES cuda_architecture_flags "${arch_flags}")
    set(VTKm_CUDA_Architecture_Flags "${arch_flags}")
  endif()
endif()

if(NOT TARGET Threads::Threads)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
endif()
