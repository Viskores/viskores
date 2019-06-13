##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

include(CMakeParseArguments)

include(VTKmDeviceAdapters)
include(VTKmCPUVectorization)

#-----------------------------------------------------------------------------
# Utility to build a kit name from the current directory.
function(vtkm_get_kit_name kitvar)
  # Will this always work?  It should if ${CMAKE_CURRENT_SOURCE_DIR} is
  # built from ${VTKm_SOURCE_DIR}.
  string(REPLACE "${VTKm_SOURCE_DIR}/" "" dir_prefix ${CMAKE_CURRENT_SOURCE_DIR})
  string(REPLACE "/" "_" kit "${dir_prefix}")
  set(${kitvar} "${kit}" PARENT_SCOPE)
  # Optional second argument to get dir_prefix.
  if (${ARGC} GREATER 1)
    set(${ARGV1} "${dir_prefix}" PARENT_SCOPE)
  endif (${ARGC} GREATER 1)
endfunction(vtkm_get_kit_name)

#-----------------------------------------------------------------------------
function(vtkm_pyexpander_generated_file generated_file_name)
  # If pyexpander is available, add targets to build and check
  if(PYEXPANDER_FOUND AND PYTHONINTERP_FOUND)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${generated_file_name}.checked
      COMMAND ${CMAKE_COMMAND}
        -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
        -DPYEXPANDER_COMMAND=${PYEXPANDER_COMMAND}
        -DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${generated_file_name}
        -DGENERATED_FILE=${CMAKE_CURRENT_BINARY_DIR}/${generated_file_name}
        -P ${VTKm_CMAKE_MODULE_PATH}/testing/VTKmCheckPyexpander.cmake
      MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${generated_file_name}.in
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${generated_file_name}
      COMMENT "Checking validity of ${generated_file_name}"
      )
    add_custom_target(check_${generated_file_name} ALL
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${generated_file_name}.checked
      )
  endif()
endfunction(vtkm_pyexpander_generated_file)

#-----------------------------------------------------------------------------
# This function is not needed by the core infrastructure of VTK-m
# as we now require CMake 3.11 on windows, and for tests we compile a single
# executable for all backends, instead of compiling for each backend.
# It is currently kept around so that examples which haven't been updated
# continue to work
function(vtkm_compile_as_cuda output)
  # We can't use set_source_files_properties(<> PROPERTIES LANGUAGE "CUDA")
  # for the following reasons:
  #
  # 1. As of CMake 3.10 MSBuild cuda language support has a bug where files
  #    aren't passed to nvcc with the explicit '-x cu' flag which will cause
  #    them to be compiled without CUDA actually enabled.
  # 2. If the source file is used by multiple targets(libraries/executable)
  #    they will all see the source file marked as being CUDA. This will cause
  #    tests / examples that reuse sources with different backends to use CUDA
  #    by mistake
  #
  # The result of this is that instead we will use file(GENERATE ) to construct
  # a proxy cu file
  set(_cuda_srcs )
  foreach(_to_be_cuda_file ${ARGN})
    get_filename_component(_fname_ext "${_to_be_cuda_file}" EXT)
    if(_fname_ext STREQUAL ".cu")
      list(APPEND _cuda_srcs "${_to_be_cuda_file}")
    else()
      get_filename_component(_cuda_fname "${_to_be_cuda_file}" NAME_WE)
      get_filename_component(_not_cuda_fullpath "${_to_be_cuda_file}" ABSOLUTE)
      list(APPEND _cuda_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_cuda_fname}.cu")
      file(GENERATE
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_cuda_fname}.cu
            CONTENT "#include \"${_not_cuda_fullpath}\"")
    endif()
  endforeach()
  set(${output} ${_cuda_srcs} PARENT_SCOPE)
endfunction()

#-----------------------------------------------------------------------------
function(vtkm_generate_export_header lib_name)
  # Get the location of this library in the directory structure
  # export headers work on the directory structure more than the lib_name
  vtkm_get_kit_name(kit_name dir_prefix)

  # Now generate a header that holds the macros needed to easily export
  # template classes. This
  string(TOUPPER ${kit_name} BASE_NAME_UPPER)
  set(EXPORT_MACRO_NAME "${BASE_NAME_UPPER}")

  set(EXPORT_IS_BUILT_STATIC 0)
  get_target_property(is_static ${lib_name} TYPE)
  if(${is_static} STREQUAL "STATIC_LIBRARY")
    #If we are building statically set the define symbol
    set(EXPORT_IS_BUILT_STATIC 1)
  endif()
  unset(is_static)

  get_target_property(EXPORT_IMPORT_CONDITION ${lib_name} DEFINE_SYMBOL)
  if(NOT EXPORT_IMPORT_CONDITION)
    #set EXPORT_IMPORT_CONDITION to what the DEFINE_SYMBOL would be when
    #building shared
    set(EXPORT_IMPORT_CONDITION ${kit_name}_EXPORTS)
  endif()


  configure_file(
      ${VTKm_SOURCE_DIR}/CMake/VTKmExportHeaderTemplate.h.in
      ${VTKm_BINARY_DIR}/include/${dir_prefix}/${kit_name}_export.h
    @ONLY)

  if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
    install(FILES ${VTKm_BINARY_DIR}/include/${dir_prefix}/${kit_name}_export.h
      DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${dir_prefix}
      )
  endif()

endfunction(vtkm_generate_export_header)

#-----------------------------------------------------------------------------
function(vtkm_install_headers dir_prefix)
  if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
    set(hfiles ${ARGN})
    install(FILES ${hfiles}
      DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${dir_prefix}
      )
  endif()
endfunction(vtkm_install_headers)


#-----------------------------------------------------------------------------
function(vtkm_declare_headers)
  vtkm_get_kit_name(name dir_prefix)
  vtkm_install_headers("${dir_prefix}" ${ARGN})
endfunction(vtkm_declare_headers)

#-----------------------------------------------------------------------------
# Add a VTK-m library. The name of the library will match the "kit" name
# (e.g. vtkm_rendering) unless the NAME argument is given.
#
# vtkm_library(
#   [NAME <name>]
#   SOURCES <source_list>
#   TEMPLATE_SOURCES <.hxx >
#   HEADERS <header list>
#   [WRAP_FOR_CUDA <source_list>]
#   )
function(vtkm_library)
  set(options STATIC SHARED)
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCES HEADERS TEMPLATE_SOURCES WRAP_FOR_CUDA)
  cmake_parse_arguments(VTKm_LIB
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  if(NOT VTKm_LIB_NAME)
    message(FATAL_ERROR "vtkm library must have an explicit name")
  endif()
  set(lib_name ${VTKm_LIB_NAME})

  if(VTKm_LIB_STATIC)
    set(VTKm_LIB_type STATIC)
  else()
    if(VTKm_LIB_SHARED)
      set(VTKm_LIB_type SHARED)
    endif()
    #if cuda requires static libaries force
    #them no matter what
    if(TARGET vtkm::cuda)
      get_target_property(force_static vtkm::cuda INTERFACE_REQUIRES_STATIC_BUILDS)
      if(force_static)
        set(VTKm_LIB_type STATIC)
        message("Forcing ${lib_name} to be built statically as we are using CUDA 8.X, which doesn't support virtuals sufficiently in dynamic libraries.")
      endif()
    endif()

  endif()


  if(TARGET vtkm::cuda)
    set_source_files_properties(${VTKm_LIB_WRAP_FOR_CUDA} PROPERTIES LANGUAGE "CUDA")
  endif()

  add_library(${lib_name}
              ${VTKm_LIB_type}
              ${VTKm_LIB_SOURCES}
              ${VTKm_LIB_HEADERS}
              ${VTKm_LIB_TEMPLATE_SOURCES}
              ${VTKm_LIB_WRAP_FOR_CUDA}
              )

  #when building either static or shared we want pic code
  set_target_properties(${lib_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)

  #specify when building with cuda we want separable compilation
  set_property(TARGET ${lib_name} PROPERTY CUDA_SEPARABLE_COMPILATION ON)

  #specify where to place the built library
  set_property(TARGET ${lib_name} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${lib_name} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${lib_name} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH})

  if(NOT VTKm_USE_DEFAULT_SYMBOL_VISIBILITY)
    set_property(TARGET ${lib_name} PROPERTY CUDA_VISIBILITY_PRESET "hidden")
    set_property(TARGET ${lib_name} PROPERTY CXX_VISIBILITY_PRESET "hidden")
  endif()

  # allow the static cuda runtime find the driver (libcuda.dyllib) at runtime.
  if(APPLE)
    set_property(TARGET ${lib_name} PROPERTY BUILD_RPATH ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
  endif()

  # Setup the SOVERSION and VERSION information for this vtkm library
  set_property(TARGET ${lib_name} PROPERTY VERSION 1)
  set_property(TARGET ${lib_name} PROPERTY SOVERSION 1)

  # Support custom library suffix names, for other projects wanting to inject
  # their own version numbers etc.
  if(DEFINED VTKm_CUSTOM_LIBRARY_SUFFIX)
    set(_lib_suffix "${VTKm_CUSTOM_LIBRARY_SUFFIX}")
  else()
    set(_lib_suffix "-${VTKm_VERSION_MAJOR}.${VTKm_VERSION_MINOR}")
  endif()
  set_property(TARGET ${lib_name} PROPERTY OUTPUT_NAME ${lib_name}${_lib_suffix})

  #generate the export header and install it
  vtkm_generate_export_header(${lib_name})

  #install the headers
  vtkm_declare_headers(${VTKm_LIB_HEADERS}
                       ${VTKm_LIB_TEMPLATE_SOURCES})

  # When building libraries/tests that are part of the VTK-m repository inherit
  # the properties from vtkm_developer_flags. The flags are intended only for
  # VTK-m itself and are not needed by consumers. We will export
  # vtkm_developer_flags so consumer can use VTK-m's build flags if they so
  # desire
  if (VTKm_ENABLE_DEVELOPER_FLAGS)
    target_link_libraries(${lib_name} PUBLIC $<BUILD_INTERFACE:vtkm_developer_flags>)
  else()
    target_link_libraries(${lib_name} PRIVATE $<BUILD_INTERFACE:vtkm_developer_flags>)
  endif()

  #install the library itself
  install(TARGETS ${lib_name}
    EXPORT ${VTKm_EXPORT_NAME}
    ARCHIVE DESTINATION ${VTKm_INSTALL_LIB_DIR}
    LIBRARY DESTINATION ${VTKm_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${VTKm_INSTALL_BIN_DIR}
    )

endfunction(vtkm_library)

#-----------------------------------------------------------------------------
# Declare unit tests, which should be in the same directory as a kit
# (package, module, whatever you call it).  Usage:
#
# vtkm_unit_tests(
#   NAME
#   SOURCES <source_list>
#   BACKEND <type>
#   LIBRARIES <dependent_library_list>
#   DEFINES <target_compile_definitions>
#   TEST_ARGS <argument_list>
#   MPI
#   ALL_BACKENDS
#   <options>
#   )
#
# [BACKEND]: mark all source files as being compiled with the proper defines
#            to make this backend the default backend
#            If the backend is specified as CUDA it will also imply all
#            sources should be treated as CUDA sources
#            The backend name will also be added to the executable name
#            so you can test multiple backends easily
#
# [LIBRARIES] : extra libraries that this set of tests need to link too
#
# [DEFINES]   : extra defines that need to be set for all unit test sources
#
# [TEST_ARGS] : arguments that should be passed on the command line to the
#               test executable
#
# [MPI]       : when specified, the tests should be run in parallel if
#               MPI is enabled.
# [ALL_BACKENDS] : when specified, the tests would test against all enabled
#                  backends. BACKEND argument would be ignored.
#
function(vtkm_unit_tests)
  if (NOT VTKm_ENABLE_TESTING)
    return()
  endif()

  set(options)
  set(global_options ${options} MPI ALL_BACKENDS)
  set(oneValueArgs BACKEND NAME)
  set(multiValueArgs SOURCES LIBRARIES DEFINES TEST_ARGS)
  cmake_parse_arguments(VTKm_UT
    "${global_options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )
  vtkm_parse_test_options(VTKm_UT_SOURCES "${options}" ${VTKm_UT_SOURCES})

  set(test_prog)
  set(backend ${VTKm_UT_BACKEND})

  set(enable_all_backends ${VTKm_UT_ALL_BACKENDS})
  set(all_backends Serial)
  if (VTKm_ENABLE_CUDA)
    list(APPEND all_backends Cuda)
  endif()
  if (VTKm_ENABLE_TBB)
    list(APPEND all_backends TBB)
  endif()
  if (VTKm_ENABLE_OPENMP)
    list(APPEND all_backends OpenMP)
  endif()

  if(VTKm_UT_NAME)
    set(test_prog "${VTKm_UT_NAME}")
  else()
    vtkm_get_kit_name(kit)
    set(test_prog "UnitTests_${kit}")
  endif()

  if(backend)
    set(test_prog "${test_prog}_${backend}")
    set(all_backends ${backend})
  elseif(NOT enable_all_backends)
    set (all_backends "NO_BACKEND")
  endif()

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
  #if all backends are enabled, we can use cuda compiler to handle all possible backends.
  if(TARGET vtkm::cuda AND (backend STREQUAL "Cuda" OR enable_all_backends))
    set_source_files_properties(${VTKm_UT_SOURCES} PROPERTIES LANGUAGE "CUDA")
  endif()

  add_executable(${test_prog} ${test_prog}.cxx ${VTKm_UT_SOURCES})
  set_property(TARGET ${test_prog} PROPERTY CUDA_SEPARABLE_COMPILATION ON)

  set_property(TARGET ${test_prog} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${test_prog} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${test_prog} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH})

  #Starting in CMake 3.13, cmake will properly drop duplicate libraries
  #from the link line so this workaround can be dropped
  if (CMAKE_VERSION VERSION_LESS 3.13 AND "vtkm_rendering" IN_LIST VTKm_UT_LIBRARIES)
    list(REMOVE_ITEM VTKm_UT_LIBRARIES "vtkm_cont")
    target_link_libraries(${test_prog} PRIVATE ${VTKm_UT_LIBRARIES})
  else()
    target_link_libraries(${test_prog} PRIVATE vtkm_cont ${VTKm_UT_LIBRARIES})
  endif()

  target_compile_definitions(${test_prog} PRIVATE ${VTKm_UT_DEFINES})

  foreach(current_backend ${all_backends})
    set (device_command_line_argument --device=${current_backend})
    if (current_backend STREQUAL "NO_BACKEND")
      set (current_backend "")
      set(device_command_line_argument "")
    endif()
    string(TOUPPER "${current_backend}" upper_backend)
    foreach (test ${VTKm_UT_SOURCES})
      get_filename_component(tname ${test} NAME_WE)
      if(VTKm_UT_MPI AND VTKm_ENABLE_MPI)
        add_test(NAME ${tname}${upper_backend}
          COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 3 ${MPIEXEC_PREFLAGS}
                  $<TARGET_FILE:${test_prog}> ${tname} ${device_command_line_argument} ${VTKm_UT_TEST_ARGS}
                  ${MPIEXEC_POSTFLAGS}
          )
      else()
        add_test(NAME ${tname}${upper_backend}
          COMMAND ${test_prog} ${tname} ${device_command_line_argument} ${VTKm_UT_TEST_ARGS}
          )
      endif()

      #determine the timeout for all the tests based on the backend. CUDA tests
      #generally require more time because of kernel generation.
      if (current_backend STREQUAL "Cuda")
        set(timeout 1500)
      else()
        set(timeout 180)
      endif()
      if(current_backend STREQUAL "OpenMP")
        #We need to have all OpenMP tests run serially as they
        #will uses all the system cores, and we will cause a N*N thread
        #explosion which causes the tests to run slower than when run
        #serially
        set(run_serial True)
      else()
        set(run_serial False)
      endif()

      set_tests_properties("${tname}${upper_backend}" PROPERTIES
        TIMEOUT ${timeout}
        RUN_SERIAL ${run_serial}
      )
    endforeach (test)
  endforeach(current_backend)

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
