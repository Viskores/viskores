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
#2. Set BOOST_SP_DISABLE_THREADS to disable threading warnings
#3. Disable unused function warnings
#   the FindCUDA module and helper methods don't read target level
#   properties so we have to modify CUDA_NVCC_FLAGS  instead of using
#   target and source level COMPILE_FLAGS and COMPILE_DEFINITIONS
#4. Set the compile option /bigobj when using VisualStudio generators.
#   While we have specified this as target compile flag, those aren't
#   currently loooked at by FindCUDA, so we have to manually add it ourselves
function(vtkm_setup_nvcc_flags old_flags )
  set(${old_flags} ${CUDA_NVCC_FLAGS} PARENT_SCOPE)
  set(new_flags ${CUDA_NVCC_FLAGS})
  list(APPEND new_flags "-DVTKM_DEVICE_ADAPTER=VTKM_DEVICE_ADAPTER_CUDA")
  list(APPEND new_flags "-DBOOST_SP_DISABLE_THREADS")
  list(APPEND new_flags "-w")
  if(MSVC)
    list(APPEND new_flags "--compiler-options;/bigobj")
  endif()
  set(CUDA_NVCC_FLAGS ${new_flags} PARENT_SCOPE)
endfunction(vtkm_setup_nvcc_flags)

#Utility to set MSVC only COMPILE_DEFINITIONS and COMPILE_FLAGS needed to
#reduce number of warnings and compile issues with Visual Studio
function(vtkm_setup_msvc_properties target )
  #disable MSVC CRT and SCL warnings as they recommend using non standard
  #c++ extensions
  set_property(TARGET ${target}
               APPEND PROPERTY COMPILE_DEFINITIONS
               "_SCL_SECURE_NO_WARNINGS"
               "_CRT_SECURE_NO_WARNINGS"
               )

  #enable large object support so we can have 2^32 addressable sections
  set_property(TARGET ${target}
               APPEND PROPERTY COMPILE_FLAGS
               "/bigobj"
               )
endfunction(vtkm_setup_msvc_properties)

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
    cuda_add_library(TestBuild_${name} ${cxxfiles} ${hfiles})
  elseif (${cxxfiles_len} GREATER 0)
    add_library(TestBuild_${name} ${cxxfiles} ${hfiles})
    if(VTKm_EXTRA_COMPILER_WARNINGS)
      set_property(TARGET ${test_prog}
                   APPEND PROPERTY COMPILE_FLAGS
                   ${CMAKE_CXX_FLAGS_WARN_EXTRA}
                   )
    endif(VTKm_EXTRA_COMPILER_WARNINGS)
  endif ()
  set_source_files_properties(${hfiles}
    PROPERTIES HEADER_FILE_ONLY TRUE
    )
endfunction(vtkm_add_header_build_test)

function(vtkm_install_headers dir_prefix)
  set(hfiles ${ARGN})
  install(FILES ${hfiles}
    DESTINATION ${VTKm_INSTALL_INCLUDE_DIR}/${dir_prefix}
    )
endfunction(vtkm_install_headers)

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
  if(PYEXPANDER_FOUND)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${generated_file_name}.checked
      COMMAND ${CMAKE_COMMAND}
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
#   )
function(vtkm_unit_tests)
  set(options CUDA)
  set(oneValueArgs)
  set(multiValueArgs SOURCES LIBRARIES)
  cmake_parse_arguments(VTKm_UT
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  #set up what we possibly need to link too.
  list(APPEND VTKm_UT_LIBRARIES ${VTKm_LIBRARIES})
  #set up storage for the include dirs
  set(VTKm_UT_INCLUDE_DIRS )

  if(VTKm_ENABLE_OPENGL_INTEROP)
    list(APPEND VTKm_UT_INCLUDE_DIRS ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} )
    list(APPEND VTKm_UT_LIBRARIES ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )
  endif()

  if(VTKm_ENABLE_OPENGL_TESTS)
    list(APPEND VTKm_UT_INCLUDE_DIRS ${GLUT_INCLUDE_DIR} )
    list(APPEND VTKm_UT_LIBRARIES ${GLUT_LIBRARIES}  )
  endif()

  if (VTKm_ENABLE_TESTING)
    vtkm_get_kit_name(kit)
    #we use UnitTests_kit_ so that it is an unique key to exclude from coverage
    set(test_prog UnitTests_kit_${kit})
    create_test_sourcelist(TestSources ${test_prog}.cxx ${VTKm_UT_SOURCES})

    #determine the timeout for all the tests based on the backend. CUDA tests
    #generally require more time because of kernel generation.
    set(timeout 180)

    if (VTKm_UT_CUDA)
      #specify a longer timeout for cuda
      set(timeout 360)

      vtkm_setup_nvcc_flags( old_nvcc_flags )

      cuda_add_executable(${test_prog} ${TestSources})

      set(CUDA_NVCC_FLAGS ${old_nvcc_flags})

    else (VTKm_UT_CUDA)
      add_executable(${test_prog} ${TestSources})
    endif (VTKm_UT_CUDA)

    #do it as a property value so we don't pollute the include_directories
    #for any other targets
    set_property(TARGET ${test_prog} APPEND PROPERTY
        INCLUDE_DIRECTORIES ${VTKm_UT_INCLUDE_DIRS} )

    target_link_libraries(${test_prog} ${VTKm_UT_LIBRARIES})

    if(MSVC)
      vtkm_setup_msvc_properties(${test_prog})
    endif()

    if(VTKm_EXTRA_COMPILER_WARNINGS)
      set_property(TARGET ${test_prog}
                 APPEND PROPERTY COMPILE_FLAGS
                 ${CMAKE_CXX_FLAGS_WARN_EXTRA}
                 )
    endif(VTKm_EXTRA_COMPILER_WARNINGS)

    foreach (test ${VTKm_UT_SOURCES})
      get_filename_component(tname ${test} NAME_WE)
      add_test(NAME ${tname}
        COMMAND ${test_prog} ${tname}
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

  #determine the timeout for all the tests based on the backend. CUDA tests
  #generally require more time because of kernel generation.
  set(timeout 180)
  if(is_cuda)
    set(timeout 360)
  endif()

  if(VTKm_ENABLE_TESTING)
    string(REPLACE "VTKM_DEVICE_ADAPTER_" "" device_type ${device_adapter})

    vtkm_get_kit_name(kit)

    #inject the device adapter into the test program name so each one is unique
    set(test_prog WorkletTests_${device_type})


    if(is_cuda)
      get_property(unit_test_srcs GLOBAL PROPERTY vtkm_worklet_unit_tests_cu_sources )
      vtkm_setup_nvcc_flags( old_nvcc_flags )

      cuda_add_executable(${test_prog} ${unit_test_drivers} ${unit_test_srcs})

      set(CUDA_NVCC_FLAGS ${old_nvcc_flags} )
    else()
      add_executable(${test_prog} ${unit_test_drivers} ${unit_test_srcs})
      target_link_libraries(${test_prog} ${VTKm_LIBRARIES})
    endif()

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

    if(MSVC)
      vtkm_setup_msvc_properties(${test_prog})
    endif()

    #increase warning level if needed, we are going to skip cuda here
    #to remove all the false positive unused function warnings that cuda
    #generates
    if(VTKm_EXTRA_COMPILER_WARNINGS)
      set_property(TARGET ${test_prog}
                   APPEND PROPERTY COMPILE_FLAGS ${CMAKE_CXX_FLAGS_WARN_EXTRA} )
    endif()

    #set the device adapter on the executable
    set_property(TARGET ${test_prog}
                 APPEND
                 PROPERTY COMPILE_DEFINITIONS "VTKM_DEVICE_ADAPTER=${device_adapter}" )
  endif()
endfunction(vtkm_worklet_unit_tests)

# Save the benchmarks to run with each device adapter
# This is based on vtkm_save_worklet_unit_tests
# Usage:
#
# vtkm_save_benchmarks( sources )
#
# notes: will save the sources absolute path as the
# vtkm_benchmarks_sources global property
function(vtkm_save_benchmarks)

  #create the benchmarks driver when we are called, since
  #the driver expects the files to be in the same
  #directory as the test driver
        #TODO: This is probably ok to use for benchmarks as well
  create_test_sourcelist(bench_sources BenchmarkDriver.cxx ${ARGN})

  #store the absolute path for the driver and all the test
  #files
  set(driver ${CMAKE_CURRENT_BINARY_DIR}/BenchmarkDriver.cxx)
  set(cxx_sources)
  set(cu_sources)

  #we need to store the absolute source for the file so that
  #we can properly compile it into the benchmark driver. At
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
                PROPERTY vtkm_benchmarks_sources ${cxx_sources})
  set_property( GLOBAL APPEND
                PROPERTY vtkm_benchmarks_cu_sources ${cu_sources})
  set_property( GLOBAL APPEND
                PROPERTY vtkm_benchmarks_drivers ${driver})

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

  set(benchmark_drivers)
  get_property(benchmark_drivers GLOBAL
               PROPERTY vtkm_benchmarks_drivers )

  #detect if we are generating a .cu files
  set(is_cuda FALSE)
  set(old_nvcc_flags ${CUDA_NVCC_FLAGS})
  if("${device_adapter}" STREQUAL "VTKM_DEVICE_ADAPTER_CUDA")
    set(is_cuda TRUE)
  endif()

  if(VTKm_ENABLE_BENCHMARKS AND VTKm_ENABLE_TESTING)
    string(REPLACE "VTKM_DEVICE_ADAPTER_" "" device_type ${device_adapter})

    vtkm_get_kit_name(kit)

    #inject the device adapter into the benchmark program name so each one is unique
    set(benchmark_prog Benchmarks_${device_type})

    if(is_cuda)
      vtkm_setup_nvcc_flags( old_nvcc_flags )
      get_property(benchmark_srcs GLOBAL PROPERTY vtkm_benchmarks_cu_sources )
      cuda_add_executable(${benchmark_prog} ${benchmark_drivers} ${benchmark_srcs})
      set(CUDA_NVCC_FLAGS ${old_nvcc_flags})
    else()
      add_executable(${benchmark_prog} ${benchmark_drivers} ${benchmark_srcs})
      target_link_libraries(${benchmark_prog} ${VTKm_LIBRARIES})
    endif()

    if(MSVC)
      vtkm_setup_msvc_properties(${benchmark_prog})
    endif()

    #increase warning level if needed, we are going to skip cuda here
    #to remove all the false positive unused function warnings that cuda
    #generates
    if(VTKm_EXTRA_COMPILER_WARNINGS)
      set_property(TARGET ${benchmark_prog}
                   APPEND PROPERTY COMPILE_FLAGS ${CMAKE_CXX_FLAGS_WARN_EXTRA} )
    endif()

    #set the device adapter on the executable
    set_property(TARGET ${benchmark_prog}
                 APPEND
                 PROPERTY COMPILE_DEFINITIONS "VTKM_DEVICE_ADAPTER=${device_adapter}" )
  endif()

endfunction(vtkm_benchmarks)

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

# Set up configuration for a given device.
macro(vtkm_configure_device device)
  string(TOUPPER "${device}" device_uppercase)
  set(VTKm_ENABLE_${device_uppercase} ON)
  include("UseVTKm${device}")
  if(NOT VTKm_${device}_FOUND)
    if ("${ARGV1}" STREQUAL "REQUIRED")
      message(SEND_ERROR "Could not configure for using VTKm with ${device}")
    else()
      message(STATUS "Could not configure for using VTKm with ${device}")
    endif()
  endif()
endmacro(vtkm_configure_device)

