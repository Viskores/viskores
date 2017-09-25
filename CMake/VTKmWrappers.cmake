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

include(VTKmBackends)

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
  #This should go in a separate cmake file
endfunction(vtkm_pyexpander_generated_file)

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
      ${VTKm_SOURCE_DIR}/cmake/VTKmExportHeaderTemplate.h.in
      ${VTKm_BINARY_DIR}/include/${dir_prefix}/${kit_name}_export.h
    @ONLY)

  install(FILES ${VTKm_BINARY_DIR}/include/${dir_prefix}/${kit_name}_export.h
    DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${dir_prefix}
    )

endfunction(vtkm_generate_export_header)

function(vtkm_install_headers dir_prefix)
  set(hfiles ${ARGN})
  install(FILES ${hfiles}
    DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${dir_prefix}
    )
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

  set(hfiles ${VTKm_DH_UNPARSED_ARGUMENTS})
  vtkm_get_kit_name(name dir_prefix)

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

  if(VTKm_ENABLE_CUDA)
    set_source_files_properties(${VTKm_LIB_WRAP_FOR_CUDA} LANGUAGE "CUDA")
  endif()


  add_library(${lib_name}
              ${VTKm_LIB_SOURCES}
              ${VTKm_LIB_HEADERS}
              ${VTKm_LIB_TEMPLATE_SOURCES}
              ${VTKm_LIB_WRAP_FOR_CUDA}
              )

  set_target_properties(${lib_name}
                        PROPERTIES
                        CXX_VISIBILITY_PRESET "hidden")

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
# [CUDA]: mark all source files as being compiled with the cuda compiler
# [BACKEND]: mark all source files as being compiled with the proper defines
#            to make this backend the default backend
#            If the backend is specified as CUDA it will also imply all
#            sources should be treated as CUDA sources
#            The backend name will also be added to the executable name
#            so you can test multiple backends easily
# vtkm_unit_tests(
#   NAME
#   CUDA
#   SOURCES <source_list>
#   BACKEND <type>
#   LIBRARIES <dependent_library_list>
#   TEST_ARGS <argument_list>
#   )
function(vtkm_unit_tests)
  set(options CUDA NO_TESTS)
  set(oneValueArgs BACKEND NAME)
  set(multiValueArgs SOURCES LIBRARIES TEST_ARGS)
  cmake_parse_arguments(VTKm_UT
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  if (NOT VTKm_ENABLE_TESTING)
    return()
  endif()

  set(backend )
  if(VTKm_UT_BACKEND)
    set(backend "_${VTKm_UT_BACKEND}")
    if(backend STREQUAL "CUDA")
      set(VTKm_UT_CUDA "TRUE")
    endif()
  endif()

  vtkm_get_kit_name(kit)
  #we use UnitTests_ so that it is an unique key to exclude from coverage
  set(test_prog "UnitTests_${kit}${backend}")
  if(VTKm_UT_NAME)
    set(test_prog "${VTKm_UT_NAME}${backend}")
  endif()


  create_test_sourcelist(TestSources ${test_prog}.cxx ${VTKm_UT_SOURCES})

  add_executable(${test_prog} ${TestSources})
  set_target_properties(${test_prog} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
    LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
    RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
    )

  target_link_libraries(${test_prog} PRIVATE vtkm)

  if(NOT VTKm_UT_NO_TESTS)
    return()
  endif()

  #determine the timeout for all the tests based on the backend. CUDA tests
  #generally require more time because of kernel generation.
  set(timeout 180)
  if(VTKm_UT_CUDA)
    set(timeout 1500)
  endif()
  foreach (test ${VTKm_UT_SOURCES})
    get_filename_component(tname ${test} NAME_WE)
    add_test(NAME ${tname}
      COMMAND ${test_prog} ${tname} ${VTKm_UT_TEST_ARGS}
      )
    set_tests_properties("${tname}" PROPERTIES TIMEOUT ${timeout})
  endforeach (test)

endfunction(vtkm_unit_tests)

#-----------------------------------------------------------------------------
# Declare benchmarks, which use all the same infrastructure as tests but
# don't actually do the add_test at the end
#
# [BACKEND]: mark all source files as being compiled with the proper defines
#            to make this backend the default backend
#            If the backend is specified as CUDA it will also imply all
#            sources should be treated as CUDA sources
#            The backend name will also be added to the executable name
#            so you can test multiple backends easily
# vtkm_benchmarks(
#   SOURCES <source_list>
#   BACKEND <type>
#   LIBRARIES <dependent_library_list>
function(vtkm_benchmarks)
  vtkm_unit_tests(NAME Benchmarks NO_TESTS ${ARGN})
endfunction(vtkm_benchmarks)
