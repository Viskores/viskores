##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 Sandia Corporation.
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

if (VTKm_CUDA_initialize_complete)
  return()
endif (VTKm_CUDA_initialize_complete)

vtkm_configure_device(Base)

if (VTKm_Base_FOUND)

  set(VTKm_CUDA_FOUND ${VTKm_ENABLE_CUDA})
  if (NOT VTKm_CUDA_FOUND)
    message(STATUS "This build of VTK-m does not include CUDA.")
  endif ()

  #---------------------------------------------------------------------------
  # Find CUDA library.
  #---------------------------------------------------------------------------
  if (VTKm_CUDA_FOUND)
    find_package(CUDA)
    mark_as_advanced(CUDA_BUILD_CUBIN
                     CUDA_BUILD_EMULATION
                     CUDA_HOST_COMPILER
                     CUDA_SDK_ROOT_DIR
                     CUDA_SEPARABLE_COMPILATION
                     CUDA_TOOLKIT_ROOT_DIR
                     CUDA_VERBOSE_BUILD
                     )

    if (NOT CUDA_FOUND)
      message(STATUS "CUDA not found")
      set(VTKm_CUDA_FOUND)
    endif ()
  endif ()

  if(VTKm_CUDA_FOUND)
  #---------------------------------------------------------------------------
  # Setup build flags for CUDA
  #---------------------------------------------------------------------------
  # Populates CUDA_NVCC_FLAGS with the best set of flags to compile for a
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
  #   - Uses: --generate-code arch=compute_20,code=compute_20
  # 3 - kepler
  #   - Uses: --generate-code arch=compute_30,code=compute_30
  #   - Uses: --generate-code arch=compute_35,code=compute_35
  # 4 - maxwell
  #   - Uses: --generate-code arch=compute_50,code=compute_50
  #   - Uses: --generate-code arch=compute_52,code=compute_52
  # 5 - all
  #   - Uses: --generate-code arch=compute_20,code=compute_20
  #   - Uses: --generate-code arch=compute_30,code=compute_30
  #   - Uses: --generate-code arch=compute_35,code=compute_35
  #   - Uses: --generate-code arch=compute_50,code=compute_50
  #

    #specify the property
    set(VTKm_CUDA_Architecture "native" CACHE STRING "Which GPU Architecture(s) to compile for")
    set_property(CACHE VTKm_CUDA_Architecture PROPERTY STRINGS native fermi kepler maxwell all)

    #detect what the propery is set too
    if(VTKm_CUDA_Architecture STREQUAL "native")
      #run execute_process to do auto_detection
      execute_process(COMMAND "${CUDA_NVCC_EXECUTABLE}" "-ccbin" "${CMAKE_CXX_COMPILER}" "--run" "${CMAKE_CURRENT_LIST_DIR}/VTKmDetectCUDAVersion.cxx"
                      RESULT_VARIABLE ran_properly
                      OUTPUT_VARIABLE run_output)

      if(ran_properly EQUAL 0)
        #find the position of the "--generate-code" output. With some compilers such as
        #msvc we get compile output plus run output. So we need to strip out just the
        #run output
        string(FIND "${run_output}" "--generate-code" position)
        string(SUBSTRING "${run_output}" ${position} -1 run_output)
        set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} ${run_output}")
      else()
        message(STATUS "Unable to run \"${CUDA_NVCC_EXECUTABLE}\" to autodetect GPU architecture."
                       "Falling back to fermi, please manually specify if you want something else.")
        set(VTKm_CUDA_Architecture "fermi")
      endif()
    endif()

    #since when we are native we can fail, and fall back to "fermi" these have
    #to happen after, and separately of the native check
    if(VTKm_CUDA_Architecture STREQUAL "fermi")
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} --generate-code arch=compute_20,code=compute_20")
    elseif(VTKm_CUDA_Architecture STREQUAL "kepler")
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} --generate-code arch=compute_30,code=compute_30")
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} --generate-code arch=compute_35,code=compute_35")
    elseif(VTKm_CUDA_Architecture STREQUAL "maxwell")
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} --generate-code arch=compute_50,code=compute_50")
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} --generate-code arch=compute_52,code=compute_52")
    elseif(VTKm_CUDA_Architecture STREQUAL "all")
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} --generate-code arch=compute_20,code=compute_20")
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} --generate-code arch=compute_30,code=compute_30")
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} --generate-code arch=compute_35,code=compute_35")
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} --generate-code arch=compute_50,code=compute_50")
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} --generate-code arch=compute_52,code=compute_52")
    endif()
  endif()

  #---------------------------------------------------------------------------
  # Find Thrust library.
  #---------------------------------------------------------------------------
  if (VTKm_CUDA_FOUND)
    find_package(Thrust)

    if (NOT THRUST_FOUND)
      message(STATUS "Thrust not found")
      set(VTKm_CUDA_FOUND)
    endif ()
  endif ()

endif () # VTKm_Base_FOUND

#-----------------------------------------------------------------------------
# Set up all these dependent packages (if they were all found).
#-----------------------------------------------------------------------------
if (VTKm_CUDA_FOUND)
  set(VTKm_INCLUDE_DIRS
    ${VTKm_INCLUDE_DIRS}
    ${THRUST_INCLUDE_DIRS}
    )

  set(VTKm_CUDA_initialize_complete TRUE)
endif (VTKm_CUDA_FOUND)
