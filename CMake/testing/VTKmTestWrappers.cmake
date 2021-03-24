##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

include(VTKmWrappers)

function(vtkm_create_test_executable
  prog_name
  sources
  libraries
  defines
  is_mpi_test
  use_mpi
  enable_all_backends
  use_job_pool)

  vtkm_diy_use_mpi_push()

  set(prog ${prog_name})

  # for MPI tests, suffix test name and add MPI_Init/MPI_Finalize calls.
  if (is_mpi_test)
    set(extraArgs EXTRA_INCLUDE "vtkm/thirdparty/diy/environment.h")

    if (use_mpi)
      vtkm_diy_use_mpi(ON)
      set(prog "${prog}_mpi")
    else()
      vtkm_diy_use_mpi(OFF)
      set(prog "${prog}_nompi")
    endif()
  else()
    set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "")
  endif()

  #the creation of the test source list needs to occur before the labeling as
  #cuda. This is so that we get the correctly named entry points generated
  create_test_sourcelist(test_sources ${prog}.cxx ${sources} ${extraArgs})

  add_executable(${prog} ${prog}.cxx ${sources})
  vtkm_add_drop_unused_function_flags(${prog})
  target_compile_definitions(${prog} PRIVATE ${defines})

  #determine if we have a device that requires a separate compiler enabled
  set(device_lang_enabled FALSE)
  if( (TARGET vtkm::cuda) OR (TARGET vtkm::kokkos_cuda) OR (TARGET vtkm::kokkos_hip))
    set(device_lang_enabled TRUE)
  endif()

  #if all backends are enabled, we can use the device compiler to handle all possible backends.
  set(device_sources)
  if(device_lang_enabled AND enable_all_backends)
    set(device_sources ${sources})
  endif()
  vtkm_add_target_information(${prog} DEVICE_SOURCES ${device_sources})

  if(NOT VTKm_USE_DEFAULT_SYMBOL_VISIBILITY)
    set_property(TARGET ${prog} PROPERTY CUDA_VISIBILITY_PRESET "hidden")
    set_property(TARGET ${prog} PROPERTY CXX_VISIBILITY_PRESET "hidden")
  endif()
  set_property(TARGET ${prog} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${prog} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${prog} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH})

  target_link_libraries(${prog} PRIVATE vtkm_cont_testing ${libraries})

  if(use_job_pool)
    vtkm_setup_job_pool()
    set_property(TARGET ${prog} PROPERTY JOB_POOL_COMPILE vtkm_pool)
  endif()

  vtkm_diy_use_mpi_pop()
endfunction()

#-----------------------------------------------------------------------------
# Declare unit tests, which should be in the same directory as a kit
# (package, module, whatever you call it).  Usage:
#
# vtkm_unit_tests(
#   NAME
#   SOURCES <source_list>
#   LIBRARIES <dependent_library_list>
#   DEFINES <target_compile_definitions>
#   TEST_ARGS <argument_list>
#   MPI
#   ALL_BACKENDS
#   USE_VTKM_JOB_POOL
#   <options>
#   )
#
# [LIBRARIES] : extra libraries that this set of tests need to link too
#
# [DEFINES]   : extra defines that need to be set for all unit test sources
#
# [LABEL]     : CTest Label to associate to this set of tests
#
# [TEST_ARGS] : arguments that should be passed on the command line to the
#               test executable
#
# [MPI]       : when specified, the tests should be run in parallel if
#               MPI is enabled. The tests should also be able to build and run
#               When MPI is not available, i.e., they should not make explicit
#               use of MPI and instead completely rely on DIY.
# [ALL_BACKENDS] : when specified, the tests would test against all enabled
#                  backends. Otherwise we expect the tests to manage the
#                  backends at runtime.
#
function(vtkm_unit_tests)
  if (NOT VTKm_ENABLE_TESTING)
    return()
  endif()

  set(options)
  set(global_options ${options} USE_VTKM_JOB_POOL MPI ALL_BACKENDS)
  set(oneValueArgs BACKEND NAME LABEL)
  set(multiValueArgs SOURCES LIBRARIES DEFINES TEST_ARGS)
  cmake_parse_arguments(VTKm_UT
    "${global_options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )
  vtkm_parse_test_options(VTKm_UT_SOURCES "${options}" ${VTKm_UT_SOURCES})

  set(per_device_command_line_arguments "NONE")
  set(per_device_suffix "")
  set(per_device_timeout 180)
  set(per_device_serial FALSE)

  set(enable_all_backends ${VTKm_UT_ALL_BACKENDS})
  if(enable_all_backends)
    set(per_device_command_line_arguments --device=serial)
    set(per_device_suffix "SERIAL")
    if (VTKm_ENABLE_CUDA)
      list(APPEND per_device_command_line_arguments --device=cuda)
      list(APPEND per_device_suffix "CUDA")
      #CUDA tests generally require more time because of kernel generation.
      list(APPEND per_device_timeout 1500)
      list(APPEND per_device_serial FALSE)
    endif()
    if (VTKm_ENABLE_TBB)
      list(APPEND per_device_command_line_arguments --device=tbb)
      list(APPEND per_device_suffix "TBB")
      list(APPEND per_device_timeout 180)
      list(APPEND per_device_serial FALSE)
    endif()
    if (VTKm_ENABLE_OPENMP)
      list(APPEND per_device_command_line_arguments --device=openmp)
      list(APPEND per_device_suffix "OPENMP")
      list(APPEND per_device_timeout 180)
      #We need to have all OpenMP tests run serially as they
      #will uses all the system cores, and we will cause a N*N thread
      #explosion which causes the tests to run slower than when run
      #serially
      list(APPEND per_device_serial TRUE)
    endif()
    if (VTKm_ENABLE_KOKKOS)
      list(APPEND per_device_command_line_arguments --device=kokkos)
      list(APPEND per_device_suffix "KOKKOS")
      #may require more time because of kernel generation.
      list(APPEND per_device_timeout 1500)
      list(APPEND per_device_serial FALSE)
    endif()
  endif()

  set(test_prog)
  if(VTKm_UT_NAME)
    set(test_prog "${VTKm_UT_NAME}")
  else()
    vtkm_get_kit_name(kit)
    set(test_prog "UnitTests_${kit}")
  endif()

  # For Testing Purposes, we will set the default logging level to INFO
  list(APPEND vtkm_default_test_log_level "-v" "INFO")

  # Add the path to the data directory so tests can find and use data files for testing
  list(APPEND VTKm_UT_TEST_ARGS "--data-dir=${VTKm_SOURCE_DIR}/data/data")

  # Add the path to the location where regression test images are to be stored
  list(APPEND VTKm_UT_TEST_ARGS "--baseline-dir=${VTKm_SOURCE_DIR}/data/baseline")

  # Add the path to the location where generated regression test images should be written
  list(APPEND VTKm_UT_TEST_ARGS "--write-dir=${VTKm_BINARY_DIR}")

  if(VTKm_UT_MPI)
    if (VTKm_ENABLE_MPI)
      vtkm_create_test_executable(
        ${test_prog}
        "${VTKm_UT_SOURCES}"
        "${VTKm_UT_LIBRARIES}"
        "${VTKm_UT_DEFINES}"
        ON   # is_mpi_test
        ON   # use_mpi
        ${enable_all_backends}
        ${VTKm_UT_USE_VTKM_JOB_POOL})
    endif()
    if ((NOT VTKm_ENABLE_MPI) OR VTKm_ENABLE_DIY_NOMPI)
      vtkm_create_test_executable(
        ${test_prog}
        "${VTKm_UT_SOURCES}"
        "${VTKm_UT_LIBRARIES}"
        "${VTKm_UT_DEFINES}"
        ON   # is_mpi_test
        OFF  # use_mpi
        ${enable_all_backends}
        ${VTKm_UT_USE_VTKM_JOB_POOL})
    endif()
  else()
    vtkm_create_test_executable(
      ${test_prog}
      "${VTKm_UT_SOURCES}"
      "${VTKm_UT_LIBRARIES}"
      "${VTKm_UT_DEFINES}"
      OFF   # is_mpi_test
      OFF   # use_mpi
      ${enable_all_backends}
      ${VTKm_UT_USE_VTKM_JOB_POOL})
  endif()

  list(LENGTH per_device_command_line_arguments number_of_devices)
  foreach(index RANGE ${number_of_devices})
    if(index EQUAL number_of_devices)
      #RANGE is inclusive on both sides, and we want it to be
      #exclusive on the end ( e.g. for(i=0; i < n; ++i))
      break()
    endif()
    if(per_device_command_line_arguments STREQUAL "NONE")
      set(device_command_line_argument)
      set(upper_backend ${per_device_suffix})
      set(timeout       ${per_device_timeout})
      set(run_serial    ${per_device_serial})
    else()
      list(GET per_device_command_line_arguments ${index} device_command_line_argument)
      list(GET per_device_suffix  ${index}  upper_backend)
      list(GET per_device_timeout ${index}  timeout)
      list(GET per_device_serial  ${index}  run_serial)
    endif()

    foreach (test ${VTKm_UT_SOURCES})
      get_filename_component(tname ${test} NAME_WE)
      if(VTKm_UT_MPI)
        if (VTKm_ENABLE_MPI)
          add_test(NAME ${tname}${upper_backend}_mpi
            COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 3 ${MPIEXEC_PREFLAGS}
                    $<TARGET_FILE:${test_prog}_mpi> ${tname} ${device_command_line_argument}
                    ${vtkm_default_test_log_level} ${VTKm_UT_TEST_ARGS} ${MPIEXEC_POSTFLAGS}
            )
          set_tests_properties("${tname}${upper_backend}_mpi" PROPERTIES
            LABELS "${upper_backend};${VTKm_UT_LABEL}"
            TIMEOUT ${timeout}
            RUN_SERIAL ${run_serial}
            FAIL_REGULAR_EXPRESSION "runtime error")
        endif() # VTKm_ENABLE_MPI
        if ((NOT VTKm_ENABLE_MPI) OR VTKm_ENABLE_DIY_NOMPI)
          add_test(NAME ${tname}${upper_backend}_nompi
            COMMAND ${test_prog}_nompi ${tname} ${device_command_line_argument}
                    ${vtkm_default_test_log_level} ${VTKm_UT_TEST_ARGS}
            )
          set_tests_properties("${tname}${upper_backend}_nompi" PROPERTIES
            LABELS "${upper_backend};${VTKm_UT_LABEL}"
            TIMEOUT ${timeout}
            RUN_SERIAL ${run_serial}
            FAIL_REGULAR_EXPRESSION "runtime error")

        endif() # VTKm_ENABLE_DIY_NOMPI
      else() # VTKm_UT_MPI
        add_test(NAME ${tname}${upper_backend}
          COMMAND ${test_prog} ${tname} ${device_command_line_argument}
                  ${vtkm_default_test_log_level} ${VTKm_UT_TEST_ARGS}
          )
        set_tests_properties("${tname}${upper_backend}" PROPERTIES
            LABELS "${upper_backend};${VTKm_UT_LABEL}"
            TIMEOUT ${timeout}
            RUN_SERIAL ${run_serial}
            FAIL_REGULAR_EXPRESSION "runtime error")
      endif() # VTKm_UT_MPI
    endforeach()
  endforeach()

endfunction(vtkm_unit_tests)

# -----------------------------------------------------------------------------
# vtkm_parse_test_options(varname options)
#   INTERNAL: Parse options specified for individual tests.
#
#   Parses the arguments to separate out options specified after the test name
#   separated by a comma e.g.
#
#   TestName,Option1,Option2
#
#   For every option in options, this will set _TestName_Option1,
#   _TestName_Option2, etc in the parent scope.
#
function(vtkm_parse_test_options varname options)
  set(names)
  foreach(arg IN LISTS ARGN)
    set(test_name ${arg})
    set(test_options)
    if(test_name AND "x${test_name}" MATCHES "^x([^,]*),(.*)$")
      set(test_name "${CMAKE_MATCH_1}")
      string(REPLACE "," ";" test_options "${CMAKE_MATCH_2}")
    endif()
    foreach(opt IN LISTS test_options)
      list(FIND options "${opt}" index)
      if(index EQUAL -1)
        message(WARNING "Unknown option '${opt}' specified for test '${test_name}'")
      else()
        set(_${test_name}_${opt} TRUE PARENT_SCOPE)
      endif()
    endforeach()
    list(APPEND names ${test_name})
  endforeach()
  set(${varname} ${names} PARENT_SCOPE)
endfunction()
