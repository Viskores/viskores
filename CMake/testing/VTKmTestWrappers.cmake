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
#               MPI is enabled.
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

  set(test_prog)


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
  endif()

  if(VTKm_UT_NAME)
    set(test_prog "${VTKm_UT_NAME}")
  else()
    vtkm_get_kit_name(kit)
    set(test_prog "UnitTests_${kit}")
  endif()

  # For Testing Purposes, we will set the default logging level to INFO
  list(APPEND vtkm_default_test_log_level "-v" "INFO")

  if(VTKm_UT_MPI)
    # for MPI tests, suffix test name and add MPI_Init/MPI_Finalize calls.
    set(test_prog "${test_prog}_mpi")
    set(extraArgs EXTRA_INCLUDE "vtkm/cont/testing/Testing.h"
                  FUNCTION "vtkm::cont::testing::Environment env")
  else()
    set(extraArgs)
  endif()

  #the creation of the test source list needs to occur before the labeling as
  #cuda. This is so that we get the correctly named entry points generated
  create_test_sourcelist(test_sources ${test_prog}.cxx ${VTKm_UT_SOURCES} ${extraArgs})

  add_executable(${test_prog} ${test_prog}.cxx ${VTKm_UT_SOURCES})
  vtkm_add_drop_unused_function_flags(${test_prog})
  target_compile_definitions(${test_prog} PRIVATE ${VTKm_UT_DEFINES})


  #if all backends are enabled, we can use cuda compiler to handle all possible backends.
  set(device_sources )
  if(TARGET vtkm::cuda AND enable_all_backends)
    set(device_sources ${VTKm_UT_SOURCES})
  endif()
  vtkm_add_target_information(${test_prog} DEVICE_SOURCES ${device_sources})

  if(NOT VTKm_USE_DEFAULT_SYMBOL_VISIBILITY)
    set_property(TARGET ${test_prog} PROPERTY CUDA_VISIBILITY_PRESET "hidden")
    set_property(TARGET ${test_prog} PROPERTY CXX_VISIBILITY_PRESET "hidden")
  endif()
  set_property(TARGET ${test_prog} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${test_prog} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${test_prog} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH})

  target_link_libraries(${test_prog} PRIVATE vtkm_cont ${VTKm_UT_LIBRARIES})

  if(VTKm_UT_USE_VTKM_JOB_POOL)
    vtkm_setup_job_pool()
    set_property(TARGET ${test_prog} PROPERTY JOB_POOL_COMPILE vtkm_pool)
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
      if(VTKm_UT_MPI AND VTKm_ENABLE_MPI)
        add_test(NAME ${tname}${upper_backend}
          COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 3 ${MPIEXEC_PREFLAGS}
                  $<TARGET_FILE:${test_prog}> ${tname} ${device_command_line_argument}
                  ${vtkm_default_test_log_level} ${VTKm_UT_TEST_ARGS} ${MPIEXEC_POSTFLAGS}
          )
      else()
        add_test(NAME ${tname}${upper_backend}
          COMMAND ${test_prog} ${tname} ${device_command_line_argument}
                  ${vtkm_default_test_log_level} ${VTKm_UT_TEST_ARGS}
          )
      endif()

      set_tests_properties("${tname}${upper_backend}" PROPERTIES
        LABELS "${upper_backend};${VTKm_UT_LABEL}"
        TIMEOUT ${timeout}
        RUN_SERIAL ${run_serial}
        FAIL_REGULAR_EXPRESSION "runtime error"
      )
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
