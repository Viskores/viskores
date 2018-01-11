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
function(vtkm_pyexpander_generated_file)
  # If pyexpander is available, add targets to build and check
  if(PYEXPANDER_FOUND AND PYTHONINTERP_FOUND)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${generated_file_name}.checked
      COMMAND ${CMAKE_COMMAND}
        -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
        -DPYEXPANDER_COMMAND=${PYEXPANDER_COMMAND}
        -DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${generated_file_name}
        -DGENERATED_FILE=${CMAKE_CURRENT_BINARY_DIR}/${generated_file_name}
        -P ${VTKm_CMAKE_MODULE_PATH}/VTKmCheckPyexpander.cmake
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
function(vtkm_compile_as_cuda output)
  # We cant use set_source_files_properties(<> PROPERTIES LANGUAGE "CUDA")
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
  foreach(_not_cuda_file ${ARGN})
    get_filename_component(_cuda_fname "${_not_cuda_file}" NAME_WE)
    get_filename_component(_not_cuda_fullpath "${_not_cuda_file}" ABSOLUTE)
    list(APPEND _cuda_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_cuda_fname}.cu")
    file(GENERATE
          OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_cuda_fname}.cu
          CONTENT "#include \"${_not_cuda_fullpath}\"")
  endforeach()
  set(${output} ${_cuda_srcs} PARENT_SCOPE)
endfunction()

#-----------------------------------------------------------------------------
function(vtkm_add_header_build_test name dir_prefix use_cuda)
  set(hfiles ${ARGN})

  set(ext "cxx")
  if(use_cuda)
    set(ext "cu")
  endif()

  set(valid_hfiles )
  set(srcs)
  foreach (header ${hfiles})
    get_source_file_property(cant_be_tested ${header} VTKm_CANT_BE_HEADER_TESTED)
    if( cant_be_tested )
      get_filename_component(headername ${header} NAME_WE)

      #By using file generate we will not trigger CMake execution when
      #a header gets touched
      file(GENERATE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TB_${headername}.${ext}
        CONTENT "
//mark that we are including headers as test for completeness.
//This is used by headers that include thrust to properly define a proper
//device backend / system
#define VTKM_TEST_HEADER_BUILD
#include <${dir_prefix}/${headername}.h>"
        )
      list(APPEND srcs ${src})
      list(APPEND valid_hfiles ${header})
    endif()
  endforeach()

  set_source_files_properties(${valid_hfiles}
    PROPERTIES HEADER_FILE_ONLY TRUE
    )

  #only attempt to add a test build executable if we have any headers to
  #test. this might not happen when everything depends on thrust.
  list(LENGTH srcs num_srcs)
  if (${num_srcs} EQUAL 0)
    return()
  endif()

  if(TARGET TestBuild_${name})
    #If the target already exists just add more sources to it
    target_sources(TestBuild_${name} PRIVATE ${srcs})
  else()
    add_library(TestBuild_${name} STATIC ${srcs} ${valid_hfiles})
    target_link_libraries(TestBuild_${name} PRIVATE vtkm_compiler_flags)

    if(TARGET vtkm::tbb)
      #make sure that we have the tbb include paths when tbb is enabled.
      target_link_libraries(TestBuild_${name} PRIVATE vtkm::tbb)
    endif()

    if(TARGET vtkm_diy)
      target_link_libraries(TestBuild_${name} PRIVATE vtkm_diy)
    endif()

    # Send the libraries created for test builds to their own directory so as to
    # not polute the directory with useful libraries.
    set_target_properties(TestBuild_${name} PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}/testbuilds
      LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}/testbuilds
      RUNTIME_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}/testbuilds
      )
  endif()

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
  #TODO: look at the testable and cuda options
  set(options CUDA)
  set(oneValueArgs TESTABLE)
  set(multiValueArgs)
  cmake_parse_arguments(VTKm_DH "${options}"
    "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  #The testable keyword allows the caller to turn off the header testing,
  #mainly used so that backends can be installed even when they can't be
  #built on the machine.
  #Since this is an optional property not setting it means you do want testing
  if(NOT DEFINED VTKm_DH_TESTABLE)
      set(VTKm_DH_TESTABLE ON)
  endif()

  set(hfiles ${VTKm_DH_UNPARSED_ARGUMENTS})
  vtkm_get_kit_name(name dir_prefix)

  #only do header testing if enable testing is turned on
  if (VTKm_ENABLE_TESTING AND VTKm_DH_TESTABLE)
    vtkm_add_header_build_test(
      "${name}" "${dir_prefix}" "${VTKm_DH_CUDA}" ${hfiles})
  endif()

  vtkm_install_headers("${dir_prefix}" ${hfiles})
endfunction(vtkm_declare_headers)

#-----------------------------------------------------------------------------
function(vtkm_install_template_sources)
  set(hfiles ${ARGN})
  vtkm_get_kit_name(name dir_prefix)

  # CMake does not add installed files as project files, and template sources
  # are not declared as source files anywhere, add a fake target here to let
  # an IDE know that these sources exist.
  add_custom_target(${name}_template_srcs SOURCES ${hfiles})

  vtkm_install_headers("${dir_prefix}" ${hfiles})
endfunction(vtkm_install_template_sources)

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

  if(TARGET vtkm::cuda)
    vtkm_compile_as_cuda(cu_srcs ${VTKm_LIB_WRAP_FOR_CUDA})
    set(VTKm_LIB_WRAP_FOR_CUDA ${cu_srcs})
  endif()


  add_library(${lib_name}
              ${VTKm_LIB_SOURCES}
              ${VTKm_LIB_HEADERS}
              ${VTKm_LIB_TEMPLATE_SOURCES}
              ${VTKm_LIB_WRAP_FOR_CUDA}
              )

  if(VTKm_USE_DEFAULT_SYMBOL_VISIBILITY)
    set_target_properties(${lib_name}
                          PROPERTIES
                          CUDA_VISIBILITY_PRESET "hidden"
                          CXX_VISIBILITY_PRESET "hidden")
  endif()


  vtkm_generate_export_header(${lib_name})
  install(TARGETS ${lib_name}
    EXPORT ${VTKm_EXPORT_NAME}
    ARCHIVE DESTINATION ${VTKm_INSTALL_LIB_DIR}
    LIBRARY DESTINATION ${VTKm_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${VTKm_INSTALL_BIN_DIR}
    )

  #test and install the headers
  vtkm_declare_headers(${VTKm_LIB_HEADERS})
  #install the template sources
  vtkm_install_template_sources(${VTKm_LIB_TEMPLATE_SOURCES})

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
#   TEST_ARGS <argument_list>
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
# [TEST_ARGS] : arguments that should be passed on the command line to the
#               test executable
#
# Supported <options> are documented below. These can be specified for
# all tests or for individual tests. When specifying these for individual tests,
# simply add them after the test name in the <source_list> separated by a comma.
# e.g. `UnitTestMultiBlock,MPI`.
#
# Supported <options> are
# * MPI : the test(s) will be executed using `mpirun`.
#
function(vtkm_unit_tests)
  set(options MPI)
  set(oneValueArgs BACKEND NAME)
  set(multiValueArgs SOURCES LIBRARIES TEST_ARGS)
  cmake_parse_arguments(VTKm_UT
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  if (NOT VTKm_ENABLE_TESTING)
    return()
  endif()

  vtkm_parse_test_options(VTKm_UT_SOURCES "${options}" ${VTKm_UT_SOURCES})

  set(backend )
  if(VTKm_UT_BACKEND)
    set(backend "_${VTKm_UT_BACKEND}")
  endif()

  vtkm_get_kit_name(kit)
  #we use UnitTests_ so that it is an unique key to exclude from coverage
  set(test_prog "UnitTests_${kit}${backend}")
  if(VTKm_UT_NAME)
    set(test_prog "${VTKm_UT_NAME}${backend}")
  endif()

  #the creation of the test source list needs to occur before the labeling as
  #cuda. This is so that we get the correctly named entry points generated
  #
  create_test_sourcelist(test_sources ${test_prog}.cxx ${VTKm_UT_SOURCES})
  if(VTKm_UT_BACKEND STREQUAL "CUDA")
    vtkm_compile_as_cuda(cu_srcs ${VTKm_UT_SOURCES})
    set(VTKm_UT_SOURCES ${cu_srcs})
  endif()

  add_executable(${test_prog} ${test_sources})
  set_target_properties(${test_prog} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
    LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
    RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
    )

  target_link_libraries(${test_prog} PRIVATE vtkm_cont ${VTKm_UT_LIBRARIES})

  if(VTKm_UT_NO_TESTS)
    return()
  endif()

  #determine the timeout for all the tests based on the backend. CUDA tests
  #generally require more time because of kernel generation.
  set(timeout 180)
  if(VTKm_UT_BACKEND STREQUAL "CUDA")
    set(timeout 1500)
  endif()
  foreach (test ${VTKm_UT_SOURCES})
    get_filename_component(tname ${test} NAME_WE)
    add_test(NAME ${tname}${backend}
      COMMAND ${test_prog} ${tname} ${VTKm_UT_TEST_ARGS}
      )
    set_tests_properties("${tname}${backend}" PROPERTIES TIMEOUT ${timeout})
  endforeach (test)

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
