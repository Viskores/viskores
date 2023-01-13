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

include(VTKmCMakeBackports)
include(VTKmDeviceAdapters)
include(VTKmCPUVectorization)

if(VTKm_ENABLE_MPI AND NOT TARGET MPI::MPI_CXX)
  find_package(MPI REQUIRED MODULE)
endif()

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
  endif ()
endfunction(vtkm_get_kit_name)

#-----------------------------------------------------------------------------
function(vtkm_pyexpander_generated_file generated_file_name)
  # If pyexpander is available, add targets to build and check
  if(PYEXPANDER_FOUND AND TARGET Python::Interpreter)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${generated_file_name}.checked
      COMMAND ${CMAKE_COMMAND}
        -DPYTHON_EXECUTABLE=${Python_EXECUTABLE}
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
# Internal function that parses a C++ header file and extract explicit
# template instantiations.
#
# USAGE:
#
# _vtkm_extract_instantiations(
#   instantiations   # Out: List of instantiations (; are escaped to $)
#   filter_header    # In:  The path of the header file to parse.
#   )
#
function(_vtkm_extract_instantiations instantiations filter_header)
  file(STRINGS "${filter_header}" read_file)

  foreach(line ${read_file})
    if("${line}" MATCHES "VTKM_INSTANTIATION_BEGIN")
      set(buf "")
    endif()

    # Escape semicolon to zip line in list
    string(REPLACE ";" "$" line "${line}")
    list(APPEND buf ${line})

    if("${line}" MATCHES "VTKM_INSTANTIATION_END")
      list(JOIN buf "\n" buf)

      # Extract, prepare, and store the instantiation
      if(${buf} MATCHES
          "VTKM_INSTANTIATION_BEGIN(.*)VTKM_INSTANTIATION_END")

        set(buf ${CMAKE_MATCH_1})

        # Remove heading and trailing spaces and newlines
        string(REGEX REPLACE "(^[ \n]+)|([ \n]+$)|([ ]*extern[ ]*)" "" buf ${buf})
        string(REPLACE "_TEMPLATE_EXPORT" "_EXPORT" buf ${buf})

        list(APPEND _instantiations ${buf})
      endif()
    endif()

  endforeach(line)

  set(${instantiations} "${_instantiations}" PARENT_SCOPE)
endfunction(_vtkm_extract_instantiations)

#-----------------------------------------------------------------------------
function(vtkm_generate_export_header lib_name)
  # Get the location of this library in the directory structure
  # export headers work on the directory structure more than the lib_name
  vtkm_get_kit_name(kit_name dir_prefix)

  # Now generate a header that holds the macros needed to easily export
  # template classes. This
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
      ${VTKm_BINARY_DIR}/include/${dir_prefix}/${lib_name}_export.h
    @ONLY)

  if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
    install(FILES ${VTKm_BINARY_DIR}/include/${dir_prefix}/${lib_name}_export.h
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
# If VTK-m was built with CMake 3.18+ and you are using CMake 3.18+ and have
# a cmake_minimum_required of 3.18 or have set policy CMP0105 to new, this will
# return an empty string as the `vtkm_cuda` target will correctly propagate
# all the necessary flags.
#
# This is required for CMake < 3.18 as they don't support the `$<DEVICE_LINK>`
# generator expression for `target_link_options`. Instead they need to be
# specified in CMAKE_CUDA_FLAGS
#
#
# add_library(lib_that_uses_vtkm ...)
# vtkm_add_cuda_flags(CMAKE_CUDA_FLAGS)
# target_link_libraries(lib_that_uses_vtkm PRIVATE vtkm_filter)
#
function(vtkm_get_cuda_flags settings_var)

  if(TARGET vtkm_cuda)
    if(POLICY CMP0105)
      cmake_policy(GET CMP0105 does_device_link)
      get_property(arch_flags
        TARGET vtkm_cuda
        PROPERTY INTERFACE_LINK_OPTIONS)
      if(arch_flags AND CMP0105 STREQUAL "NEW")
        return()
      endif()
    endif()

    get_property(arch_flags
      TARGET    vtkm_cuda
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
# This function takes a target name and returns the mangled version of its name
# in a form that complies with the VTK-m export target naming scheme.
macro(vtkm_target_mangle output target)
  string(REPLACE "vtkm_" "" ${output} ${target})
endmacro()

#-----------------------------------------------------------------------------
# Enable VTK-m targets to be installed.
#
# This function decorates the `install` CMake function mangling the exported
# target names to comply with the VTK-m exported target names scheme. Use this
# function instead of the canonical CMake `install` function for VTK-m targets.
#
# Signature:
# vtkm_install_targets(
#   TARGETS <target[s]>
#   [EXPORT <export_name>]
#   [ARGS <cmake_install_args>]
#   )
#
# Usage:
#   add_library(vtkm_library)
#   vtkm_install_targets(TARGETS vtkm_library ARGS COMPONENT core)
#
# TARGETS: List of targets to be installed.
#
# EXPORT:  [OPTIONAL] The name of the export set to which this target will be
#          added. If omitted vtkm_install_targets will use the value of
#          VTKm_EXPORT_NAME by default.
#
# ARGS:    [OPTIONAL] Any argument other than `TARGETS` and `EXPORT` accepted
#          by the `install` CMake function:
#          <https://cmake.org/cmake/help/latest/command/install.html>
#
function(vtkm_install_targets)
  set(oneValueArgs EXPORT)
  set(multiValueArgs TARGETS ARGS)
  cmake_parse_arguments(VTKm_INSTALL "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(export_name ${VTKm_EXPORT_NAME})
  if(VTKm_INSTALL_EXPORT)
    set(export_name ${VTKm_INSTALL_EXPORT})
  endif()

  if(NOT DEFINED VTKm_INSTALL_TARGETS)
    message(FATAL_ERROR "vtkm_install_targets invoked without TARGETS arguments.")
  endif()

  if(DEFINED VTKm_INSTALL_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "vtkm_install_targets missing ARGS keyword prepending install arguments")
  endif()

  foreach(tgt IN LISTS VTKm_INSTALL_TARGETS)
    vtkm_target_mangle(tgt_mangled ${tgt})
    set_target_properties(${tgt} PROPERTIES EXPORT_NAME ${tgt_mangled})
  endforeach()

  install(TARGETS ${VTKm_INSTALL_TARGETS} EXPORT ${export_name} ${VTKm_INSTALL_ARGS})
endfunction(vtkm_install_targets)

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
#  that VTK-m was compiled with.
#
#  If VTK-m was built with CMake 3.18+ and you are using CMake 3.18+ and have
#  a cmake_minimum_required of 3.18 or have set policy CMP0105 to new, this will
#  return an empty string as the `vtkm_cuda` target will correctly propagate
#  all the necessary flags.
#
#  Note: calling `vtkm_add_target_information` multiple times with
#  `MODIFY_CUDA_FLAGS` will cause duplicate compiler flags. To resolve this issue
#  you can; pass all targets and sources to a single `vtkm_add_target_information`
#  call, have the first one use `MODIFY_CUDA_FLAGS`, or use the provided
#  standalone `vtkm_get_cuda_flags` function.
#
#  DEVICE_SOURCES: The collection of source files that are used by `target(s)` that
#  need to be marked as going to a special compiler for certain device adapters
#  such as CUDA. A source file generally needs to be in DEVICE_SOURCES if (and
#  usually only if) it includes vtkm/cont/DeviceAdapterAlgorithm.h (either directly
#  or indirectly). The most common code to include DeviceAdapterAlgorithm.h are
#  those that use vtkm::cont::Algorithm or those that define worklets. Templated
#  code that does computation often links to device adapter algorithms. Some
#  device adapters that require a special compiler for device code will check in
#  their headers that a device compiler is being used when it is needed. Such
#  errors can be corrected by adding the source code to `DEVICE_SOURCES` (or
#  removing the dependence on device algorithm when possible).
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
    vtkm_get_cuda_flags(cuda_flags)
    if(cuda_flags)
      set(CMAKE_CUDA_FLAGS ${cuda_flags} PARENT_SCOPE)
    endif()
  endif()

  set(targets ${uses_vtkm_target})
  foreach(item IN LISTS VTKm_TI_UNPARSED_ARGUMENTS)
    if(TARGET ${item})
      list(APPEND targets ${item})
    endif()
  endforeach()

  if(VTKm_TI_DROP_UNUSED_SYMBOLS)
    foreach(target IN LISTS targets)
      vtkm_add_drop_unused_function_flags(${target})
    endforeach()
  endif()

  if(TARGET vtkm_cuda OR TARGET vtkm::cuda OR TARGET vtkm_kokkos_cuda OR TARGET vtkm::kokkos_cuda)
    set_source_files_properties(${VTKm_TI_DEVICE_SOURCES} PROPERTIES LANGUAGE "CUDA")
  elseif(TARGET vtkm_kokkos_hip OR TARGET vtkm::kokkos_hip)
    set_source_files_properties(${VTKm_TI_DEVICE_SOURCES} PROPERTIES LANGUAGE "HIP")
    kokkos_compilation(SOURCE ${VTKm_TI_DEVICE_SOURCES})
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

  # Skip unity builds unless explicitly asked
  foreach(source IN LISTS VTKm_LIB_SOURCES VTKm_LIB_DEVICE_SOURCES)
    get_source_file_property(is_candidate ${source} UNITY_BUILD_CANDIDATE)
    if (NOT is_candidate)
      list(APPEND non_unity_sources ${source})
    endif()
  endforeach()

  set_source_files_properties(${non_unity_sources} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

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
  if(VTKm_HIDE_PRIVATE_SYMBOLS)
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

  if (NOT VTKm_SKIP_LIBRARY_VERSIONS)
    # Setup the SOVERSION and VERSION information for this vtkm library
    set_property(TARGET ${lib_name} PROPERTY VERSION ${VTKm_VERSION}.${VTKm_VERSION_PATCH})
    set_property(TARGET ${lib_name} PROPERTY SOVERSION ${VTKm_VERSION})
  endif ()

  # Support custom library suffix names, for other projects wanting to inject
  # their own version numbers etc.
  if(DEFINED VTKm_CUSTOM_LIBRARY_SUFFIX)
    set(_lib_suffix "${VTKm_CUSTOM_LIBRARY_SUFFIX}")
  else()
    set(_lib_suffix "-${VTKm_VERSION_MAJOR}.${VTKm_VERSION_MINOR}")
  endif()
  set_property(TARGET ${lib_name} PROPERTY OUTPUT_NAME ${lib_name}${_lib_suffix})

  # Include any module information
  if(vtkm_module_current)
    if(NOT lib_name STREQUAL vtkm_module_current)
      # We do want each library to be in its own module. (VTK's module allows you to declare
      # multiple libraries per module. We may want that in the future, but right now we should
      # not need it.)
      message(FATAL_ERROR
        "Library name `${lib_name}` does not match module name `${vtkm_module_current}`")
    endif()
    vtkm_module_get_property(depends ${vtkm_module_current} DEPENDS)
    vtkm_module_get_property(private_depends ${vtkm_module_current} PRIVATE_DEPENDS)
    vtkm_module_get_property(optional_depends ${vtkm_module_current} OPTIONAL_DEPENDS)
    target_link_libraries(${lib_name}
      PUBLIC ${depends}
      PRIVATE ${private_depends}
      )
    foreach(opt_dep IN LISTS optional_depends)
      if(TARGET ${opt_dep})
        target_link_libraries(${lib_name} PRIVATE ${opt_dep})
      endif()
    endforeach()
  else()
    # Might need to add an argument to vtkm_library to create an exception to this rule
    message(FATAL_ERROR "Library `${lib_name}` is not created inside of a VTK-m module.")
  endif()

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
  vtkm_install_targets(TARGETS ${lib_name} ARGS
    ARCHIVE DESTINATION ${VTKm_INSTALL_LIB_DIR}
    LIBRARY DESTINATION ${VTKm_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${VTKm_INSTALL_BIN_DIR}
    )

  if(VTKm_LIB_USE_VTKM_JOB_POOL)
    vtkm_setup_job_pool()
    set_property(TARGET ${lib_name} PROPERTY JOB_POOL_COMPILE vtkm_pool)
  endif()

endfunction(vtkm_library)

#[==[
-----------------------------------------------------------------------------
Produce _instantiation-files_ given a filter.

VTK-m makes use of a lot of headers. It is often the case when building a
library that you have to compile several instantiations of the template to
cover all the types expected. However, when you try to do this in a single
cxx file, you can end up with some very long compiles, especially when
using a GPU device compiler. In this case, it is helpful to split up the
instantiations across multiple files.

This function will parse a given header file and look for pairs of
`VTKM_INSTANTIATION_BEGIN` and `VTKM_INSTANTIATION_END`. (These are defined
in `vtkm/internal/Instantiations.h`.) Between these two macros there should
be the definition of a single extern template instantiation. The definition
needs to fully qualify the namespace of all symbols. The declaration
typically looks something like this.

```cpp
VTKM_INSTANTIATION_BEGIN
extern template vtkm::cont::ArrayHandle<vtkm::Float32> vtkm::filter::foo::RunFooWorklet(
  const vtkm::cont::CellSetExplicit<>& inCells,
  const vtkm::cont::ArrayHandle<vtkm::Vec3f_32>& inField);
VTKM_INSTANTIATION_END
```

For each one of these found, a source file will be produced that compiles
the template for the given instantiation. Those produced files are stored
in the build directory and are not versioned.

_Important note_: The `extern template` should not be of an inline function
or method. If the function or method is inline, then a compiler might compile
ts own instance of the template regardless of the known export, which defeats
the purpose of making the instances. In particular, if the `extern template`
is referring to a method, make sure the implementation for that method is
defined _outside_ of the class. Implementations defined inside of a class are
implicitly considered inline.

Usage:
   vtkm_add_instantiations(
     instantiations_list
     INSTANTIATIONS_FILE <path>
     [ TEMPLATE_SOURCE <path> ]
     )

instantiations_list: Output variable which contain the path of the newly
produced _instantiation-files_.

INSTANTIATIONS_FILE: Parameter with the relative path of the file that
contains the extern template instantiations.

TEMPLATE_SOURCE: _Optional_ parameter with the relative path to the header
file that contains the implementation of the template. If not given, the
template source is set to be the same as the INSTANTIATIONS_FILE.
#]==]
function(vtkm_add_instantiations instantiations_list)
  # Parse and validate parameters
  set(oneValueArgs INSTANTIATIONS_FILE TEMPLATE_SOURCE)
  cmake_parse_arguments(VTKm_instantiations "" "${oneValueArgs}" "" ${ARGN})

  if(NOT VTKm_instantiations_INSTANTIATIONS_FILE)
    message(FATAL_ERROR "vtkm_add_instantiations needs a valid INSTANTIATIONS_FILE parameter")
  endif()

  set(instantiations_file ${VTKm_instantiations_INSTANTIATIONS_FILE})

  if(VTKm_instantiations_TEMPLATE_SOURCE)
    set(file_template_source ${VTKm_instantiations_TEMPLATE_SOURCE})
  else()
    set(file_template_source ${instantiations_file})
  endif()

  # Extract explicit instantiations
  _vtkm_extract_instantiations(instantiations ${instantiations_file})

  # Compute relative path of header files
  file(RELATIVE_PATH INSTANTIATION_TEMPLATE_SOURCE
    ${VTKm_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/${file_template_source}"
    )

  # Make a guard macro name so that the TEMPLATE_SOURCE can determine if it is compiling
  # the instances (if necessary).
  get_filename_component(instantations_name "${instantiations_file}" NAME_WE)
  set(INSTANTIATION_INC_GUARD "vtkm_${instantations_name}Instantiation")

  # Generate instatiation file in the build directory
  set(counter 0)
  foreach(instantiation IN LISTS instantiations)
    string(REPLACE "$" ";" instantiation ${instantiation})
    set(INSTANTIATION_DECLARATION "${instantiation}")

    # Create instantiation in build directory
    set(instantiation_path
      "${CMAKE_CURRENT_BINARY_DIR}/${instantations_name}Instantiation${counter}.cxx"
      )
    configure_file("${VTKm_SOURCE_DIR}/CMake/InstantiationTemplate.cxx.in"
      ${instantiation_path}
      @ONLY
      )

    # Return value
    list(APPEND _instantiations_list ${instantiation_path})
    math(EXPR counter "${counter} + 1")
  endforeach(instantiation)

  # Force unity builds here
  set_source_files_properties(${_instantiations_list} PROPERTIES
    SKIP_UNITY_BUILD_INCLUSION OFF
    UNITY_BUILD_CANDIDATE ON
    )
  set(${instantiations_list} ${_instantiations_list} PARENT_SCOPE)
endfunction(vtkm_add_instantiations)
