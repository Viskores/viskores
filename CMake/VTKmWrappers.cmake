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
include(VTKmMPI)

#-----------------------------------------------------------------------------
# INTERNAL FUNCTIONS
# No promises when used from outside VTK-m

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
function(vtkm_setup_job_pool)
  # The VTK-m job pool is only used for components that use large amounts
  # of memory such as worklet tests, filters, and filter tests
  get_property(vtkm_pool_established
    GLOBAL PROPERTY VTKM_JOB_POOL_ESTABLISHED SET)
  if(NOT vtkm_pool_established)
    # The VTK-m filters uses large amounts of memory to compile as it does lots
    # of template expansion. To reduce the amount of tension on the machine when
    # using generators such as ninja we restrict the number of VTK-m enabled
    # compilation units to be built at the same time.
    #
    # We try to allocate a pool size where we presume each compilation process
    # will require 3GB of memory. To allow for other NON VTK-m jobs we leave at
    # least 3GB of memory as 'slop'.
    cmake_host_system_information(RESULT vtkm_mem_ QUERY TOTAL_PHYSICAL_MEMORY)
    math(EXPR vtkm_pool_size "(${vtkm_mem_}/3072)-1")

    if (vtkm_pool_size LESS 1)
      set(vtkm_pool_size 1)
    endif ()

    set_property(GLOBAL APPEND
      PROPERTY
        JOB_POOLS vtkm_pool=${vtkm_pool_size})
    set_property(GLOBAL PROPERTY VTKM_JOB_POOL_ESTABLISHED TRUE)
  endif()
endfunction()

#-----------------------------------------------------------------------------
# FORWARD FACING API

#-----------------------------------------------------------------------------
# Pass to consumers extra compile flags they need to add to CMAKE_CUDA_FLAGS
# to have CUDA compatibility.
#
# This is required as currently the -sm/-gencode flags when specified inside
# COMPILE_OPTIONS / target_compile_options are not propagated to the device
# linker. Instead they must be specified in CMAKE_CUDA_FLAGS
#
#
# add_library(lib_that_uses_vtkm ...)
# vtkm_add_cuda_flags(CMAKE_CUDA_FLAGS)
# target_link_libraries(lib_that_uses_vtkm PRIVATE vtkm_filter)
#
function(vtkm_get_cuda_flags settings_var)
  if(TARGET vtkm::cuda)
    get_property(arch_flags
      TARGET    vtkm::cuda
      PROPERTY  cuda_architecture_flags)
    set(${settings_var} "${${settings_var}} ${arch_flags}" PARENT_SCOPE)
  endif()
endfunction()

#-----------------------------------------------------------------------------
# Add to a target linker flags that allow unused VTK-m functions to be dropped,
# which helps keep binary sizes down. This works as VTK-m is compiled with
# ffunction-sections which allows for the linker to remove unused functions.
# If you are building a program that loads runtime plugins that can call
# VTK-m this most likely shouldn't be used as symbols the plugin expects
# to exist will be removed.
#
# add_library(lib_that_uses_vtkm ...)
# vtkm_add_drop_unused_function_flags(lib_that_uses_vtkm)
# target_link_libraries(lib_that_uses_vtkm PRIVATE vtkm_filter)
#
function(vtkm_add_drop_unused_function_flags uses_vtkm_target)
  get_target_property(lib_type ${uses_vtkm_target} TYPE)
  if(${lib_type} STREQUAL "SHARED_LIBRARY" OR
     ${lib_type} STREQUAL "MODULE_LIBRARY" OR
     ${lib_type} STREQUAL "EXECUTABLE" )

    if(APPLE)
      #OSX Linker uses a different flag for this
      set_property(TARGET ${uses_vtkm_target} APPEND_STRING PROPERTY
        LINK_FLAGS " -Wl,-dead_strip")
    elseif(VTKM_COMPILER_IS_GNU OR VTKM_COMPILER_IS_CLANG)
      set_property(TARGET ${uses_vtkm_target} APPEND_STRING PROPERTY
        LINK_FLAGS " -Wl,--gc-sections")
    endif()

  endif()
endfunction()


#-----------------------------------------------------------------------------
# Add a relevant information to target that wants to use VTK-m.
#
# This higher order function allow build-systems that use VTK-m
# to use `add_library` or `add_executable` calls but still have an
# easy to way to get the required information to have VTK-m using
# compilation units compile correctly.
#
# vtkm_add_target_information(
#   target[s]
#   [ DROP_UNUSED_SYMBOLS ]
#   [ MODIFY_CUDA_FLAGS ]
#   [ EXTENDS_VTKM ]
#   [ DEVICE_SOURCES <source_list> ]
#   )
#
# Usage:
#   add_library(lib_that_uses_vtkm STATIC a.cxx)
#   vtkm_add_target_information(lib_that_uses_vtkm
#                               DROP_UNUSED_SYMBOLS
#                               MODIFY_CUDA_FLAGS
#                               DEVICE_SOURCES a.cxx
#                               )
#   target_link_libraries(lib_that_uses_vtkm PRIVATE vtkm_filter)
#
#  DROP_UNUSED_SYMBOLS: If enabled will apply the appropiate link
#  flags to drop unused VTK-m symbols. This works as VTK-m is compiled with
#  -ffunction-sections which allows for the linker to remove unused functions.
#  If you are building a program that loads runtime plugins that can call
#  VTK-m this most likely shouldn't be used as symbols the plugin expects
#  to exist will be removed.
#  Enabling this will help keep library sizes down when using static builds
#  of VTK-m as only the functions you call will be kept. This can have a
#  dramatic impact on the size of the resulting executable / shared library.
#
#
#  MODIFY_CUDA_FLAGS: If enabled will add the required -arch=<ver> flags
#  that VTK-m was compiled with. If you have multiple libraries that use
#  VTK-m calling `vtkm_add_target_information` multiple times with
#  `MODIFY_CUDA_FLAGS` will cause duplicate compiler flags. To resolve this issue
#  you can; pass all targets and sources to a single `vtkm_add_target_information`
#  call, have the first one use `MODIFY_CUDA_FLAGS`, or use the provided
#  standalone `vtkm_get_cuda_flags` function.
#
#  DEVICE_SOURCES: The collection of source files that are used by `target(s)` that
#  need to be marked as going to a special compiler for certain device adapters
#  such as CUDA.
#
#  EXTENDS_VTKM: Some programming models have restrictions on how types can be used,
#  passed across library boundaries, and derived from.
#  For example CUDA doesn't allow device side calls across dynamic library boundaries,
#  and requires all polymorphic classes to be reachable at dynamic library/executable
#  link time.
#
#  To accommodate these restrictions we need to handle the following allowable
#  use-cases:
#   Object library: do nothing, zero restrictions
#   Executable: do nothing, zero restrictions
#   Static library: do nothing, zero restrictions
#   Dynamic library:
#     -> Wanting to use VTK-m as implementation detail, doesn't expose VTK-m
#        types to consumers. This is supported no matter if CUDA is enabled.
#     -> Wanting to extend VTK-m and provide these types to consumers.
#        This is only supported when CUDA isn't enabled. Otherwise we need to ERROR!
#     -> Wanting to pass known VTK-m types across library boundaries for others
#        to use in filters/worklets.
#        This is only supported when CUDA isn't enabled. Otherwise we need to ERROR!
#
#  For most consumers they can ignore the `EXTENDS_VTKM` property as the default
#  will be correct.
#
#
function(vtkm_add_target_information uses_vtkm_target)
  set(options DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS EXTENDS_VTKM)
  set(multiValueArgs DEVICE_SOURCES)
  cmake_parse_arguments(VTKm_TI
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )


  if(VTKm_TI_MODIFY_CUDA_FLAGS)
    vtkm_get_cuda_flags(CMAKE_CUDA_FLAGS)
    set(CMAKE_CUDA_FLAGS ${CMAKE_CUDA_FLAGS} PARENT_SCOPE)
  endif()

  set(targets ${uses_vtkm_target})
  foreach(item IN LISTS VTKm_TI_UNPARSED_ARGUMENTS)
    if(TARGET ${item})
      list(APPEND targets ${item})
    endif()
  endforeach()

  # set the required target properties
  set_target_properties(${targets} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_target_properties(${targets} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

  if(VTKm_TI_DROP_UNUSED_SYMBOLS)
    foreach(target IN LISTS targets)
      vtkm_add_drop_unused_function_flags(${target})
    endforeach()
  endif()

  # Validate that following:
  #   - We are building with CUDA enabled.
  #   - We are building a VTK-m library or a library that wants cross library
  #     device calls.
  #
  # This is required as CUDA currently doesn't support device side calls across
  # dynamic library boundaries.
  if(TARGET vtkm::cuda)
    set_source_files_properties(${VTKm_TI_DEVICE_SOURCES} PROPERTIES LANGUAGE "CUDA")
    foreach(target IN LISTS targets)
      get_target_property(lib_type ${target} TYPE)
      get_target_property(requires_static vtkm::cuda requires_static_builds)

      if(requires_static AND ${lib_type} STREQUAL "SHARED_LIBRARY" AND VTKm_TI_EXTENDS_VTKM)
        #We provide different error messages based on if we are building VTK-m
        #or being called by a consumer of VTK-m. We use PROJECT_NAME so that we
        #produce the correct error message when VTK-m is a subdirectory include
        #of another project
        if(PROJECT_NAME STREQUAL "VTKm")
          message(SEND_ERROR "${target} needs to be built STATIC as CUDA doesn't"
                " support virtual methods across dynamic library boundaries. You"
                " need to set the CMake option BUILD_SHARED_LIBS to `OFF`.")
        else()
          message(SEND_ERROR "${target} needs to be built STATIC as CUDA doesn't"
                  " support virtual methods across dynamic library boundaries. You"
                  " should either explicitly call add_library with the `STATIC` keyword"
                  " or set the CMake option BUILD_SHARED_LIBS to `OFF`.")
        endif()
      endif()
    endforeach()
  endif()
endfunction()


#-----------------------------------------------------------------------------
# Add a VTK-m library. The name of the library will match the "kit" name
# (e.g. vtkm_rendering) unless the NAME argument is given.
#
# vtkm_library(
#   [ NAME <name> ]
#   [ OBJECT | STATIC | SHARED ]
#   SOURCES <source_list>
#   TEMPLATE_SOURCES <.hxx >
#   HEADERS <header list>
#   USE_VTKM_JOB_POOL
#   [ DEVICE_SOURCES <source_list> ]
#   )
function(vtkm_library)
  set(options OBJECT STATIC SHARED USE_VTKM_JOB_POOL)
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCES HEADERS TEMPLATE_SOURCES DEVICE_SOURCES)
  cmake_parse_arguments(VTKm_LIB
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  if(NOT VTKm_LIB_NAME)
    message(FATAL_ERROR "vtkm library must have an explicit name")
  endif()
  set(lib_name ${VTKm_LIB_NAME})

  if(VTKm_LIB_OBJECT)
    set(VTKm_LIB_type OBJECT)
  elseif(VTKm_LIB_STATIC)
    set(VTKm_LIB_type STATIC)
  elseif(VTKm_LIB_SHARED)
    set(VTKm_LIB_type SHARED)
  endif()

  add_library(${lib_name}
              ${VTKm_LIB_type}
              ${VTKm_LIB_SOURCES}
              ${VTKm_LIB_HEADERS}
              ${VTKm_LIB_TEMPLATE_SOURCES}
              ${VTKm_LIB_DEVICE_SOURCES}
              )
  vtkm_add_target_information(${lib_name}
                              EXTENDS_VTKM
                              DEVICE_SOURCES ${VTKm_LIB_DEVICE_SOURCES}
                              )
  if(NOT VTKm_USE_DEFAULT_SYMBOL_VISIBILITY)
    set_property(TARGET ${lib_name} PROPERTY CUDA_VISIBILITY_PRESET "hidden")
    set_property(TARGET ${lib_name} PROPERTY CXX_VISIBILITY_PRESET "hidden")
  endif()
  #specify where to place the built library
  set_property(TARGET ${lib_name} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${lib_name} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${VTKm_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${lib_name} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${VTKm_EXECUTABLE_OUTPUT_PATH})

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

  if(VTKm_LIB_USE_VTKM_JOB_POOL)
    vtkm_setup_job_pool()
    set_property(TARGET ${lib_name} PROPERTY JOB_POOL_COMPILE vtkm_pool)
  endif()

endfunction(vtkm_library)
