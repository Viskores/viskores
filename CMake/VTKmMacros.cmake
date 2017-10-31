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


#Utility to setup nvcc flags so that we properly work around issues inside FindCUDA.
#if we are generating cu files need to setup four things.
#1. Explicitly set the cuda device adapter as a define this is currently
#   done as a work around since the cuda executable ignores compile
#   definitions
#2. Disable unused function warnings
#   the FindCUDA module and helper methods don't read target level
#   properties so we have to modify CUDA_NVCC_FLAGS  instead of using
#   target and source level COMPILE_FLAGS and COMPILE_DEFINITIONS
#3. Set the compile option /bigobj when using VisualStudio generators
#   While we have specified this as target compile flag, those aren't
#   currently loooked at by FindCUDA, so we have to manually add it ourselves
function(vtkm_setup_nvcc_flags old_nvcc_flags old_cxx_flags )
  set(${old_nvcc_flags} ${CUDA_NVCC_FLAGS} PARENT_SCOPE)
  set(${old_nvcc_flags} ${CMAKE_CXX_FLAGS} PARENT_SCOPE)
  set(new_nvcc_flags ${CUDA_NVCC_FLAGS})
  set(new_cxx_flags ${CMAKE_CXX_FLAGS})
  list(APPEND new_nvcc_flags "-DVTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA")
  list(APPEND new_nvcc_flags "-w")
  if(MSVC)
    list(APPEND new_nvcc_flags "--compiler-options;/bigobj")

    # The MSVC compiler gives a warning about having two incompatiable warning
    # flags in the command line. So, ironically, adding -w above to remove
    # warnings makes MSVC give a warning. To get around that, remove all
    # warning flags from the standard CXX arguments (which are typically passed
    # to the CUDA compiler).
    string(REGEX REPLACE "[-/]W[1-4]" "" new_cxx_flags "${new_cxx_flags}")
    string(REGEX REPLACE "[-/]Wall" "" new_cxx_flags "${new_cxx_flags}")
  endif()
  set(CUDA_NVCC_FLAGS ${new_nvcc_flags} PARENT_SCOPE)
  set(CMAKE_CXX_FLAGS ${new_cxx_flags} PARENT_SCOPE)
endfunction(vtkm_setup_nvcc_flags)

#Utility to set MSVC only COMPILE_DEFINITIONS and COMPILE_FLAGS needed to
#reduce number of warnings and compile issues with Visual Studio
function(vtkm_setup_msvc_properties target )
  if(NOT MSVC)
    return()
  endif()

  #disable MSVC CRT and SCL warnings as they recommend using non standard
  #c++ extensions
  target_compile_definitions(${target} PRIVATE "_SCL_SECURE_NO_WARNINGS"
                                               "_CRT_SECURE_NO_WARNINGS")

  #C4702 Generates numerous false positives with template code about
  #      unreachable code
  #C4505 Generates numerous warnings about unused functions being
  #      removed when doing header test builds.
  target_compile_options(${target} PRIVATE -wd4702 -wd4505)

  # In VS2013 the C4127 warning has a bug in the implementation and
  # generates false positive warnings for lots of template code
  if(MSVC_VERSION LESS 1900)
    target_compile_options(${target} PRIVATE -wd4127 )
  endif()

endfunction(vtkm_setup_msvc_properties)

# vtkm_target_name(<name>)
#
# This macro does some basic checking for library naming, and also adds a suffix
# to the output name with the VTKm version by default. Setting the variable
# VTKm_CUSTOM_LIBRARY_SUFFIX will override the suffix.
function(vtkm_target_name _name)
  get_property(_type TARGET ${_name} PROPERTY TYPE)
  if(NOT "${_type}" STREQUAL EXECUTABLE)
    set_property(TARGET ${_name} PROPERTY VERSION 1)
    set_property(TARGET ${_name} PROPERTY SOVERSION 1)
  endif()
  if("${_name}" MATCHES "^[Vv][Tt][Kk][Mm]")
    set(_vtkm "")
  else()
    set(_vtkm "vtkm")
    #message(AUTHOR_WARNING "Target [${_name}] does not start in 'vtkm'.")
  endif()
  # Support custom library suffix names, for other projects wanting to inject
  # their own version numbers etc.
  if(DEFINED VTKm_CUSTOM_LIBRARY_SUFFIX)
    set(_lib_suffix "${VTKm_CUSTOM_LIBRARY_SUFFIX}")
  else()
    set(_lib_suffix "-${VTKm_VERSION_MAJOR}.${VTKm_VERSION_MINOR}")
  endif()
  set_property(TARGET ${_name} PROPERTY OUTPUT_NAME ${_vtk}${_name}${_lib_suffix})
endfunction()

function(vtkm_target _name)
  vtkm_target_name(${_name})
endfunction()

# Builds a source file and an executable that does nothing other than
# compile the given header files.
function(vtkm_add_header_build_test name dir_prefix use_cuda)
  set(hfiles ${ARGN})
  if (use_cuda)
    set(suffix ".cu")
  else (use_cuda)
    set(suffix ".cxx")
  endif (use_cuda)
  set(cxxfiles)
  foreach (header ${ARGN})
    get_source_file_property(cant_be_tested ${header} VTKm_CANT_BE_HEADER_TESTED)

    if( NOT cant_be_tested )
      string(REPLACE "${CMAKE_CURRENT_BINARY_DIR}" "" header "${header}")
      get_filename_component(headername ${header} NAME_WE)
      set(src ${CMAKE_CURRENT_BINARY_DIR}/TB_${headername}${suffix})
      configure_file(${VTKm_SOURCE_DIR}/CMake/TestBuild.cxx.in ${src} @ONLY)
      list(APPEND cxxfiles ${src})
    endif()

  endforeach (header)

  #only attempt to add a test build executable if we have any headers to
  #test. this might not happen when everything depends on thrust.
  list(LENGTH cxxfiles cxxfiles_len)
  if (use_cuda AND ${cxxfiles_len} GREATER 0)

    vtkm_setup_nvcc_flags( old_nvcc_flags old_cxx_flags )

    # Cuda compiles do not respect target_include_directories
    # and we want system includes so we have to hijack cuda
    # to do it
    foreach(dir ${VTKm_INCLUDE_DIRS})
      #this internal variable has changed names depending on the CMake ver
      list(APPEND CUDA_NVCC_INCLUDE_ARGS_USER -isystem ${dir})
      list(APPEND CUDA_NVCC_INCLUDE_DIRS_USER -isystem ${dir})
    endforeach()

    cuda_include_directories(${VTKm_SOURCE_DIR}
                             ${VTKm_BINARY_INCLUDE_DIR}
                            )

    cuda_add_library(TestBuild_${name} STATIC ${cxxfiles} ${hfiles})


    set(CUDA_NVCC_FLAGS ${old_nvcc_flags})
    set(CMAKE_CXX_FLAGS ${old_cxx_flags})

  elseif (${cxxfiles_len} GREATER 0)
    add_library(TestBuild_${name} STATIC ${cxxfiles} ${hfiles})
    target_include_directories(TestBuild_${name} PRIVATE vtkm ${VTKm_INCLUDE_DIRS})
  endif ()
  target_link_libraries(TestBuild_${name} PRIVATE vtkm_cont ${VTKm_LIBRARIES})
  set_source_files_properties(${hfiles}
    PROPERTIES HEADER_FILE_ONLY TRUE
    )

  vtkm_setup_msvc_properties(TestBuild_${name})

  # Send the libraries created for test builds to their own directory so as to
  # not polute the directory with useful libraries.
  set_target_properties(TestBuild_${name} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}/testbuilds
    LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}/testbuilds
    RUNTIME_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}/testbuilds
    )
endfunction(vtkm_add_header_build_test)

function(vtkm_install_headers dir_prefix)
  set(hfiles ${ARGN})
  install(FILES ${hfiles}
    DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${dir_prefix}
    )
endfunction(vtkm_install_headers)

function(vtkm_install_template_sources)
  vtkm_get_kit_name(name dir_prefix)
  set(hfiles ${ARGN})
  vtkm_install_headers("${dir_prefix}" ${hfiles})
  # CMake does not add installed files as project files, and template sources
  # are not declared as source files anywhere, add a fake target here to let
  # an IDE know that these sources exist.
  add_custom_target(${name}_template_srcs SOURCES ${hfiles})
endfunction(vtkm_install_template_sources)

# Declare a list of headers that require thrust to be enabled
# for them to header tested. In cases of thrust version 1.5 or less
# we have to make sure openMP is enabled, otherwise we are okay
function(vtkm_requires_thrust_to_test)
  #determine the state of thrust and testing
  set(cant_be_tested FALSE)
    if(NOT VTKm_ENABLE_THRUST)
      #mark as not valid
      set(cant_be_tested TRUE)
    elseif(NOT VTKm_ENABLE_OPENMP)
      #mark also as not valid
      set(cant_be_tested TRUE)
    endif()

  foreach(header ${ARGN})
    #set a property on the file that marks if we can header test it
    set_source_files_properties( ${header}
        PROPERTIES VTKm_CANT_BE_HEADER_TESTED ${cant_be_tested} )

  endforeach(header)

endfunction(vtkm_requires_thrust_to_test)

# Declare a list of header files.  Will make sure the header files get
# compiled and show up in an IDE.
function(vtkm_declare_headers)
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
  #always install headers
  vtkm_install_headers("${dir_prefix}" ${hfiles})
endfunction(vtkm_declare_headers)

# Declare a list of worklet files.
function(vtkm_declare_worklets)
  # Currently worklets are just really header files.
  vtkm_declare_headers(${ARGN})
endfunction(vtkm_declare_worklets)

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
        -P ${CMAKE_SOURCE_DIR}/CMake/VTKmCheckPyexpander.cmake
      MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${generated_file_name}.in
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${generated_file_name}
      COMMENT "Checking validity of ${generated_file_name}"
      )
    add_custom_target(check_${generated_file_name} ALL
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${generated_file_name}.checked
      )
  endif()
endfunction(vtkm_pyexpander_generated_file)

# Declare unit tests, which should be in the same directory as a kit
# (package, module, whatever you call it).  Usage:
#
# vtkm_unit_tests(
#   SOURCES <source_list>
#   LIBRARIES <dependent_library_list>
#   TEST_ARGS <argument_list>
#   )
function(vtkm_unit_tests)
  set(options CUDA)
  set(oneValueArgs)
  set(multiValueArgs SOURCES LIBRARIES TEST_ARGS)
  cmake_parse_arguments(VTKm_UT
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  if (VTKm_ENABLE_TESTING)
    vtkm_get_kit_name(kit)
    #we use UnitTests_ so that it is an unique key to exclude from coverage
    set(test_prog UnitTests_${kit})
    create_test_sourcelist(TestSources ${test_prog}.cxx ${VTKm_UT_SOURCES})

    #determine the timeout for all the tests based on the backend. CUDA tests
    #generally require more time because of kernel generation.
    set(timeout 180)
    if (VTKm_UT_CUDA)
      set(timeout 1500)
    endif()

    if (VTKm_UT_CUDA)
      vtkm_setup_nvcc_flags( old_nvcc_flags old_cxx_flags )

      # Cuda compiles do not respect target_include_directories
      cuda_include_directories(${VTKm_SOURCE_DIR}
                               ${VTKm_BINARY_INCLUDE_DIR}
                               ${VTKm_INCLUDE_DIRS}
                               )

      cuda_add_executable(${test_prog} ${TestSources})

      set(CUDA_NVCC_FLAGS ${old_nvcc_flags})
      set(CMAKE_CXX_FLAGS ${old_cxx_flags})

    else (VTKm_UT_CUDA)
      add_executable(${test_prog} ${TestSources})
    endif (VTKm_UT_CUDA)

    set_target_properties(${test_prog} PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
      LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
      RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
      )

    #do it as a property value so we don't pollute the include_directories
    #for any other targets
    target_include_directories(${test_prog} PRIVATE ${VTKm_INCLUDE_DIRS})

    target_link_libraries(${test_prog} PRIVATE vtkm_cont ${VTKm_LIBRARIES})

    target_compile_options(${test_prog} PRIVATE ${VTKm_COMPILE_OPTIONS})

    vtkm_setup_msvc_properties(${test_prog})

    foreach (test ${VTKm_UT_SOURCES})
      get_filename_component(tname ${test} NAME_WE)
      add_test(NAME ${tname}
        COMMAND ${test_prog} ${tname} ${VTKm_UT_TEST_ARGS}
        )
      set_tests_properties("${tname}" PROPERTIES TIMEOUT ${timeout})
    endforeach (test)
  endif (VTKm_ENABLE_TESTING)

endfunction(vtkm_unit_tests)

# Save the worklets to test with each device adapter
# Usage:
#
# vtkm_save_worklet_unit_tests( sources )
#
# notes: will save the sources absolute path as the
# vtkm_source_worklet_unit_tests global property
function(vtkm_save_worklet_unit_tests )

  #create the test driver when we are called, since
  #the test driver expect the test files to be in the same
  #directory as the test driver
  create_test_sourcelist(test_sources WorkletTestDriver.cxx ${ARGN})

  #store the absolute path for the test drive and all the test
  #files
  set(driver ${CMAKE_CURRENT_BINARY_DIR}/WorkletTestDriver.cxx)
  set(cxx_sources)
  set(cu_sources)

  #we need to store the absolute source for the file so that
  #we can properly compile it into the test driver. At
  #the same time we want to configure each file into the build
  #directory as a .cu file so that we can compile it with cuda
  #if needed
  foreach(fname ${ARGN})
    set(absPath)

    get_filename_component(absPath ${fname} ABSOLUTE)
    get_filename_component(file_name_only ${fname} NAME_WE)

    set(cuda_file_name "${CMAKE_CURRENT_BINARY_DIR}/${file_name_only}.cu")
    configure_file("${absPath}"
                   "${cuda_file_name}"
                   COPYONLY)
    list(APPEND cxx_sources ${absPath})
    list(APPEND cu_sources ${cuda_file_name})
  endforeach()

  #we create a property that holds all the worklets to test,
  #but don't actually attempt to create a unit test with the yet.
  #That is done by each device adapter
  set_property( GLOBAL APPEND
                PROPERTY vtkm_worklet_unit_tests_sources ${cxx_sources})
  set_property( GLOBAL APPEND
                PROPERTY vtkm_worklet_unit_tests_cu_sources ${cu_sources})
  set_property( GLOBAL APPEND
                PROPERTY vtkm_worklet_unit_tests_drivers ${driver})

endfunction(vtkm_save_worklet_unit_tests)

# Call each worklet test for the given device adapter
# Usage:
#
# vtkm_worklet_unit_tests( device_adapter )
#
# notes: will look for the vtkm_source_worklet_unit_tests global
# property to find what are the worklet unit tests that need to be
# compiled for the give device adapter
function(vtkm_worklet_unit_tests device_adapter)

  set(unit_test_srcs)
  get_property(unit_test_srcs GLOBAL
               PROPERTY vtkm_worklet_unit_tests_sources )

  set(unit_test_drivers)
  get_property(unit_test_drivers GLOBAL
               PROPERTY vtkm_worklet_unit_tests_drivers )

  #detect if we are generating a .cu files
  set(is_cuda FALSE)
  if("${device_adapter}" STREQUAL "VTKM_DEVICE_ADAPTER_CUDA")
    set(is_cuda TRUE)
  endif()

  #determine the timeout for all the tests based on the backend. The first CUDA
  #worklet test requires way more time because of the overhead to allow the
  #driver to convert the kernel code from virtual arch to actual arch.
  #
  set(timeout 180)
  if(is_cuda)
    set(timeout 1500)
  endif()

  if(VTKm_ENABLE_TESTING)
    string(REPLACE "VTKM_DEVICE_ADAPTER_" "" device_type ${device_adapter})

    vtkm_get_kit_name(kit)

    #inject the device adapter into the test program name so each one is unique
    set(test_prog WorkletTests_${device_type})


    if(is_cuda)
      get_property(unit_test_srcs GLOBAL PROPERTY vtkm_worklet_unit_tests_cu_sources )
      vtkm_setup_nvcc_flags( old_nvcc_flags old_cxx_flags )

      # Cuda compiles do not respect target_include_directories
      cuda_include_directories(${VTKm_SOURCE_DIR}
                               ${VTKm_BINARY_INCLUDE_DIR}
                               ${VTKm_INCLUDE_DIRS}
                               )

      cuda_add_executable(${test_prog} ${unit_test_drivers} ${unit_test_srcs})

      set(CUDA_NVCC_FLAGS ${old_nvcc_flags})
      set(CMAKE_CXX_FLAGS ${old_cxx_flags})
    else()
      add_executable(${test_prog} ${unit_test_drivers} ${unit_test_srcs})
    endif()

    set_target_properties(${test_prog} PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
      LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
      RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
      )
    target_include_directories(${test_prog} PRIVATE ${VTKm_INCLUDE_DIRS})
    target_link_libraries(${test_prog} PRIVATE vtkm_cont ${VTKm_LIBRARIES})

    #add the specific compile options for this executable
    target_compile_options(${test_prog} PRIVATE ${VTKm_COMPILE_OPTIONS})

    #add a test for each worklet test file. We will inject the device
    #adapter type into the test name so that it is easier to see what
    #exact device a test is failing on.
    foreach (test ${unit_test_srcs})
      get_filename_component(tname ${test} NAME_WE)
      add_test(NAME "${tname}${device_type}"
        COMMAND ${test_prog} ${tname}
        )

      set_tests_properties("${tname}${device_type}" PROPERTIES TIMEOUT ${timeout})
    endforeach (test)

    vtkm_setup_msvc_properties(${test_prog})

    #set the device adapter on the executable
    target_compile_definitions(${test_prog} PRIVATE "VTKM_DEVICE_ADAPTER=${device_adapter}")
  endif()
endfunction(vtkm_worklet_unit_tests)

# Save the benchmarks to run with each device adapter
# This is based on vtkm_save_worklet_unit_tests
# Usage:
#
# vtkm_save_benchmarks( <sources> [HEADERS <headers>] )
#
#
# Each benchmark source file needs to implement main(int agrc, char *argv[])
#
# notes: will save the sources absolute path as the
# vtkm_benchmarks_sources global property
function(vtkm_save_benchmarks)

  #store the absolute path for all the test files
  set(cxx_sources)
  set(cu_sources)

  cmake_parse_arguments(save_benchmarks "" "" "HEADERS" ${ARGN})

  #we need to store the absolute source for the file so that
  #we can properly compile it into the benchmark driver. At
  #the same time we want to configure each file into the build
  #directory as a .cu file so that we can compile it with cuda
  #if needed
  foreach(fname ${save_benchmarks_UNPARSED_ARGUMENTS})
    set(absPath)

    get_filename_component(absPath ${fname} ABSOLUTE)
    get_filename_component(file_name_only ${fname} NAME_WE)

    set(cuda_file_name "${CMAKE_CURRENT_BINARY_DIR}/${file_name_only}.cu")
    configure_file("${absPath}"
                   "${cuda_file_name}"
                   COPYONLY)
    list(APPEND cxx_sources ${absPath})
    list(APPEND cu_sources ${cuda_file_name})
  endforeach()

  #we create a property that holds all the worklets to test,
  #but don't actually attempt to create a unit test with the yet.
  #That is done by each device adapter
  set_property( GLOBAL APPEND
                PROPERTY vtkm_benchmarks_sources ${cxx_sources})
  set_property( GLOBAL APPEND
                PROPERTY vtkm_benchmarks_cu_sources ${cu_sources})
  set_property( GLOBAL APPEND
                PROPERTY vtkm_benchmarks_headers ${save_benchmarks_HEADERS})

endfunction(vtkm_save_benchmarks)

# Call each benchmark for the given device adapter
# Usage:
#
# vtkm_benchmark( device_adapter )
#
# notes: will look for the vtkm_benchmarks_sources global
# property to find what are the benchmarks that need to be
# compiled for the give device adapter
function(vtkm_benchmarks device_adapter)

  set(benchmark_srcs)
  get_property(benchmark_srcs GLOBAL
               PROPERTY vtkm_benchmarks_sources )

  set(benchmark_headers)
  get_property(benchmark_headers GLOBAL
               PROPERTY vtkm_benchmarks_headers )

  #detect if we are generating a .cu files
  set(is_cuda FALSE)
  set(old_nvcc_flags ${CUDA_NVCC_FLAGS})
  set(old_cxx_flags ${CMAKE_CXX_FLAGS})
  if("${device_adapter}" STREQUAL "VTKM_DEVICE_ADAPTER_CUDA")
    set(is_cuda TRUE)
  endif()

  if(VTKm_ENABLE_BENCHMARKS)
    string(REPLACE "VTKM_DEVICE_ADAPTER_" "" device_type ${device_adapter})

    if(is_cuda)
      vtkm_setup_nvcc_flags( old_nvcc_flags old_cxx_flags )
      get_property(benchmark_srcs GLOBAL PROPERTY vtkm_benchmarks_cu_sources )
    endif()

    foreach( file  ${benchmark_srcs})
      #inject the device adapter into the benchmark program name so each one is unique
      get_filename_component(benchmark_prog ${file} NAME_WE)
      set(benchmark_prog "${benchmark_prog}_${device_type}")

      if(is_cuda)
        # Cuda compiles do not respect target_include_directories

        cuda_include_directories(${VTKm_SOURCE_DIR}
                                 ${VTKm_BINARY_INCLUDE_DIR}
                                 ${VTKm_BACKEND_INCLUDE_DIRS}
                                 )

        cuda_add_executable(${benchmark_prog} ${file} ${benchmark_headers})
      else()
        add_executable(${benchmark_prog} ${file} ${benchmark_headers})
      endif()

      set_target_properties(${benchmark_prog} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
        LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
        RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
        )

      set_source_files_properties(${benchmark_headers}
        PROPERTIES HEADER_FILE_ONLY TRUE)

      target_include_directories(${benchmark_prog} PRIVATE ${VTKm_BACKEND_INCLUDE_DIRS})
      target_link_libraries(${benchmark_prog} PRIVATE vtkm_cont ${VTKm_BACKEND_LIBRARIES})

      vtkm_setup_msvc_properties(${benchmark_prog})

      #add the specific compile options for this executable
      target_compile_options(${benchmark_prog} PRIVATE ${VTKm_COMPILE_OPTIONS})

      #set the device adapter on the executable
      target_compile_definitions(${benchmark_prog} PRIVATE "VTKM_DEVICE_ADAPTER=${device_adapter}")

    endforeach()

    if(is_cuda)
      set(CUDA_NVCC_FLAGS ${old_nvcc_flags})
      set(CMAKE_CXX_FLAGS ${old_cxx_flags})
    endif()
  endif()

endfunction(vtkm_benchmarks)

# Given a list of *.cxx source files that during configure time are deterimined
# to have CUDA code, wrap the sources in *.cu files so that they get compiled
# with nvcc.
function(vtkm_wrap_sources_for_cuda cuda_source_list_var)
  set(original_sources ${ARGN})

  set(cuda_sources)
  foreach(source_file ${original_sources})
    get_filename_component(source_name ${source_file} NAME_WE)
    get_filename_component(source_file_path ${source_file} ABSOLUTE)
    set(wrapped_file ${CMAKE_CURRENT_BINARY_DIR}/${source_name}.cu)
    configure_file(
      ${VTKm_SOURCE_DIR}/CMake/WrapCUDASource.cu.in
      ${wrapped_file}
      @ONLY)
    list(APPEND cuda_sources ${wrapped_file})
  endforeach(source_file)

  # Set original sources as header files (which they basically are) so that
  # we can add them to the file list and they will show up in IDE but they will
  # not be compiled separately.
  set_source_files_properties(${original_sources}
    PROPERTIES HEADER_FILE_ONLY TRUE
    )
  set(${cuda_source_list_var} ${cuda_sources} ${original_sources} PARENT_SCOPE)
endfunction(vtkm_wrap_sources_for_cuda)

# Add a VTK-m library. The name of the library will match the "kit" name
# (e.g. vtkm_rendering) unless the NAME argument is given.
#
# vtkm_library(
#   [NAME <name>]
#   SOURCES <source_list>
#   [HEADERS <headers_list>]
#   [CUDA]
#   [WRAP_FOR_CUDA <source_list>]
#   [LIBRARIES <dependent_library_list>]
#   )
function(vtkm_library)
  set(options CUDA)
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCES HEADERS WRAP_FOR_CUDA)
  cmake_parse_arguments(VTKm_LIB
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  vtkm_get_kit_name(kit dir_prefix)
  if(VTKm_LIB_NAME)
    set(lib_name ${VTKm_LIB_NAME})
  else()
    set(lib_name ${kit})
  endif()

  list(APPEND VTKm_LIB_SOURCES ${VTKm_LIB_HEADERS})
  set_source_files_properties(${VTKm_LIB_HEADERS}
    PROPERTIES HEADER_FILE_ONLY TRUE
    )

  if(VTKm_LIB_WRAP_FOR_CUDA)
    if(VTKm_ENABLE_CUDA)
      # If we have some sources marked as WRAP_FOR_CUDA and we support CUDA,
      # then we need to turn on CDUA, wrap those sources, and add the wrapped
      # code to the sources list.
      set(VTKm_LIB_CUDA TRUE)
      vtkm_wrap_sources_for_cuda(cuda_sources ${VTKm_LIB_WRAP_FOR_CUDA})
      list(APPEND VTKm_LIB_SOURCES ${cuda_sources})
    else()
      # If we have some sources marked as WRAP_FOR_CUDA but we do not support
      # CUDA, then just compile these sources normally by adding them to the
      # sources list.
      list(APPEND VTKm_LIB_SOURCES ${VTKm_LIB_WRAP_FOR_CUDA})
    endif()
  endif()

  if(VTKm_LIB_CUDA)
    vtkm_setup_nvcc_flags(old_nvcc_flags old_cxx_flags)


    # Cuda compiles do not respect target_include_directories
    cuda_include_directories(${VTKm_SOURCE_DIR}
                             ${VTKm_BINARY_INCLUDE_DIR}
                             ${VTKm_BACKEND_INCLUDE_DIRS}
                             )

    if(BUILD_SHARED_LIBS AND NOT WIN32)
      set(compile_options -Xcompiler=${CMAKE_CXX_COMPILE_OPTIONS_VISIBILITY}hidden)
    endif()

    cuda_add_library(${lib_name} ${VTKm_LIB_SOURCES}
                     OPTIONS "${compile_options}")

    set(CUDA_NVCC_FLAGS ${old_nvcc_flags})
    set(CMAKE_CXX_FLAGS ${old_cxx_flags})
  else()
    add_library(${lib_name} ${VTKm_LIB_SOURCES})
  endif()

  vtkm_target(${lib_name})

  target_link_libraries(${lib_name} PUBLIC vtkm)
  target_link_libraries(${lib_name} PRIVATE
    ${VTKm_BACKEND_LIBRARIES}
    ${VTKm_LIB_LIBRARIES}
    )

  set(cxx_args ${VTKm_COMPILE_OPTIONS})
  separate_arguments(cxx_args)
  target_compile_options(${lib_name} PRIVATE ${cxx_args})

  # Make sure libraries go to lib directory and dll go to bin directory.
  # Mostly important on Windows.
  set_target_properties(${lib_name} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
    LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH}
    RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH}
    )

  vtkm_setup_msvc_properties(${lib_name})

  if(VTKm_EXTRA_COMPILER_WARNINGS)
    set(cxx_args ${CMAKE_CXX_FLAGS_WARN_EXTRA})
    separate_arguments(cxx_args)
    target_compile_options(${lib_name}
      PRIVATE ${cxx_args}
      )
  endif(VTKm_EXTRA_COMPILER_WARNINGS)

  #Now generate a header that holds the macros needed to easily export
  #template classes. This
  string(TOUPPER ${lib_name} BASE_NAME_UPPER)
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
    set(EXPORT_IMPORT_CONDITION ${lib_name}_EXPORTS)
  endif()

  configure_file(
      ${VTKm_SOURCE_DIR}/CMake/VTKmExportHeaderTemplate.h.in
      ${VTKm_BINARY_INCLUDE_DIR}/${dir_prefix}/${lib_name}_export.h
    @ONLY)

  unset(EXPORT_MACRO_NAME)
  unset(EXPORT_IS_BUILT_STATIC)
  unset(EXPORT_IMPORT_CONDITION)

  install(TARGETS ${lib_name}
    EXPORT ${VTKm_EXPORT_NAME}
    ARCHIVE DESTINATION ${VTKm_INSTALL_LIB_DIR}
    LIBRARY DESTINATION ${VTKm_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${VTKm_INSTALL_BIN_DIR}
    )
  vtkm_install_headers("${dir_prefix}"
    ${VTKm_BINARY_INCLUDE_DIR}/${dir_prefix}/${lib_name}_export.h
    ${VTKm_LIB_HEADERS}
    )
endfunction(vtkm_library)

# The Thrust project is not as careful as the VTKm project in avoiding warnings
# on shadow variables and unused arguments.  With a real GCC compiler, you
# can disable these warnings inline, but with something like nvcc, those
# pragmas cause errors.  Thus, this macro will disable the compiler warnings.
macro(vtkm_disable_troublesome_thrust_warnings)
  vtkm_disable_troublesome_thrust_warnings_var(CMAKE_CXX_FLAGS_DEBUG)
  vtkm_disable_troublesome_thrust_warnings_var(CMAKE_CXX_FLAGS_MINSIZEREL)
  vtkm_disable_troublesome_thrust_warnings_var(CMAKE_CXX_FLAGS_RELEASE)
  vtkm_disable_troublesome_thrust_warnings_var(CMAKE_CXX_FLAGS_RELWITHDEBINFO)
endmacro(vtkm_disable_troublesome_thrust_warnings)

macro(vtkm_disable_troublesome_thrust_warnings_var flags_var)
  set(old_flags "${${flags_var}}")
  string(REPLACE "-Wshadow" "" new_flags "${old_flags}")
  string(REPLACE "-Wunused-parameter" "" new_flags "${new_flags}")
  string(REPLACE "-Wunused" "" new_flags "${new_flags}")
  string(REPLACE "-Wextra" "" new_flags "${new_flags}")
  string(REPLACE "-Wall" "" new_flags "${new_flags}")
  set(${flags_var} "${new_flags}")
endmacro(vtkm_disable_troublesome_thrust_warnings_var)

include(VTKmConfigureComponents)
