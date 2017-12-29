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

if(VTKm_ENABLE_TBB AND NOT TARGET vtkm::tbb)
  find_package(TBB REQUIRED)

  add_library(vtkm::tbb UNKNOWN IMPORTED)

  set_target_properties(vtkm::tbb PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${TBB_INCLUDE_DIRS}")

    if(TBB_LIBRARY_RELEASE)
      set_property(TARGET vtkm::tbb APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(vtkm::tbb PROPERTIES IMPORTED_LOCATION_RELEASE "${TBB_LIBRARY_RELEASE}")
    endif()

    if(TBB_LIBRARY_DEBUG)
      set_property(TARGET vtkm::tbb APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties(vtkm::tbb PROPERTIES IMPORTED_LOCATION_DEBUG "${TBB_LIBRARY_DEBUG}")
    endif()

    if(NOT TBB_LIBRARY_RELEASE AND NOT TBB_LIBRARY_DEBUG)
      set_property(TARGET vtkm::tbb APPEND PROPERTY IMPORTED_LOCATION "${TBB_LIBRARY}")
    endif()
endif()


if(VTKm_ENABLE_CUDA AND NOT TARGET vtkm::cuda)
  cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
  enable_language(CUDA)

  add_library(vtkm::cuda UNKNOWN IMPORTED)

  set_target_properties(vtkm::cuda PROPERTIES
    INTERFACE_COMPILE_OPTIONS $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>
  )

  # We can't have this location/lib empty, so we provide a location that is
  # valid and will have no effect on compilation
  list(GET CMAKE_CUDA_IMPLICIT_LINK_LIBRARIES 0 VTKM_CUDA_LIBRARY)
  if(IS_ABSOLUTE "${VTKM_CUDA_LIBRARY}")
    set_property(TARGET vtkm::cuda APPEND PROPERTY IMPORTED_LOCATION "${VTKM_CUDA_LIBRARY}")
  else()
    find_library(cuda_lib
                 NAME ${VTKM_CUDA_LIBRARY}
                 PATHS ${CMAKE_CUDA_HOST_IMPLICIT_LINK_DIRECTORIES}
                 )
    set(VTKM_CUDA_LIBRARY ${cuda_lib})
    set_property(TARGET vtkm::cuda APPEND PROPERTY IMPORTED_LOCATION "${VTKM_CUDA_LIBRARY}")
  endif()

  set_target_properties(vtkm::cuda PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}")

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
  #
  # 1 - native
  #   - Uses system introspection to determine compile flags
  # 2 - fermi
  #   - Uses: --generate-code=arch=compute_20,code=compute_20
  # 3 - kepler
  #   - Uses: --generate-code=arch=compute_30,code=compute_30
  #   - Uses: --generate-code=arch=compute_35,code=compute_35
  # 4 - maxwell
  #   - Uses: --generate-code=arch=compute_50,code=compute_50
  #   - Uses: --generate-code=arch=compute_52,code=compute_52
  # 5 - pascal
  #   - Uses: --generate-code=arch=compute_60,code=compute_60
  #   - Uses: --generate-code=arch=compute_61,code=compute_61
  # 6 - volta
  #   - Uses: --generate-code=arch=compute_70,code=compute_70
  # 7 - all
  #   - Uses: --generate-code=arch=compute_20,code=compute_20
  #   - Uses: --generate-code=arch=compute_30,code=compute_30
  #   - Uses: --generate-code=arch=compute_35,code=compute_35
  #   - Uses: --generate-code=arch=compute_50,code=compute_50
  #   - Uses: --generate-code=arch=compute_52,code=compute_52
  #   - Uses: --generate-code=arch=compute_60,code=compute_60
  #   - Uses: --generate-code=arch=compute_61,code=compute_61
  #   - Uses: --generate-code=arch=compute_70,code=compute_70
  #

  #specify the property
  set(VTKm_CUDA_Architecture "native" CACHE STRING "Which GPU Architecture(s) to compile for")
  set_property(CACHE VTKm_CUDA_Architecture PROPERTY STRINGS native fermi kepler maxwell pascal volta all)

  #detect what the propery is set too
  if(VTKm_CUDA_Architecture STREQUAL "native")

    if(VTKM_CUDA_NATIVE_EXE_PROCESS_RAN_OUTPUT)
      #Use the cached value
      # replace any semicolons with an empty space as CMAKE_CUDA_FLAGS is
      # a string not a list and this could be cached from when it was a list
      string(REPLACE ";" " " run_output "${VTKM_CUDA_NATIVE_EXE_PROCESS_RAN_OUTPUT}")
      set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} ${run_output}")
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

        # replace any semicolons with an empty space as CMAKE_CUDA_FLAGS is
        # a string not a list
        string(REPLACE ";" " " run_output "${run_output}")
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} ${run_output}")

        set(VTKM_CUDA_NATIVE_EXE_PROCESS_RAN_OUTPUT ${run_output} CACHE INTERNAL
                "device type(s) for cuda[native]")
      else()
        set(VTKm_CUDA_Architecture "fermi")
      endif()
    endif()
  endif()

  #since when we are native we can fail, and fall back to "fermi" these have
  #to happen after, and separately of the native check
  if(VTKm_CUDA_Architecture STREQUAL "fermi")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_20,code=compute_20")
  elseif(VTKm_CUDA_Architecture STREQUAL "kepler")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_30,code=compute_30")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_35,code=compute_35")
  elseif(VTKm_CUDA_Architecture STREQUAL "maxwell")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_50,code=compute_50")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_52,code=compute_52")
  elseif(VTKm_CUDA_Architecture STREQUAL "pascal")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_60,code=compute_60")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_61,code=compute_61")
  elseif(VTKm_CUDA_Architecture STREQUAL "volta")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_70,code=compute_70")
  elseif(VTKm_CUDA_Architecture STREQUAL "all")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_30,code=compute_30")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_35,code=compute_35")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_50,code=compute_50")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_52,code=compute_52")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_60,code=compute_60")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --generate-code=arch=compute_61,code=compute_61")
  endif()

endif()
