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

#We require CMake 3.3 for Modern CMake which as features such as:
# - Better custom command support
# - Better acting if() argument expansion
# - Support for usage requirements
# -
cmake_minimum_required(VERSION 3.3)

project (VTKm)

# Update module path
set(VTKm_CMAKE_MODULE_PATH ${VTKm_SOURCE_DIR}/CMake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VTKm_CMAKE_MODULE_PATH})

# Determine VTK-m version
include(Utilities/Git/Git.cmake)
include(VTKmDetermineVersion)

# Load hardcoded version in case this is not a Git repository
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "VTKm")
# Get the version from git if we can
determine_version(${VTKm_SOURCE_DIR} ${GIT_EXECUTABLE} "VTKm")

if (NOT DEFINED VTKm_INSTALL_INCLUDE_DIR)
  set(VTKm_INSTALL_INCLUDE_DIR "include/vtkm-${VTKm_VERSION_MAJOR}.${VTKm_VERSION_MINOR}")
endif()
if (NOT DEFINED VTKm_INSTALL_CONFIG_DIR)
  set(VTKm_INSTALL_CONFIG_DIR "lib/cmake/vtkm-${VTKm_VERSION_MAJOR}.${VTKm_VERSION_MINOR}")
endif()
if (NOT DEFINED VTKm_INSTALL_LIB_DIR)
  set(VTKm_INSTALL_LIB_DIR "lib")
endif()
if (NOT DEFINED VTKm_INSTALL_BIN_DIR)
  set(VTKm_INSTALL_BIN_DIR "bin")
endif()
if (NOT DEFINED VTKm_INSTALL_CMAKE_MODULE_DIR)
  set(VTKm_INSTALL_CMAKE_MODULE_DIR "share/vtkm-${VTKm_VERSION_MAJOR}.${VTKm_VERSION_MINOR}/cmake")
endif()

set(VTKm_BINARY_INCLUDE_DIR "${VTKm_BINARY_DIR}/include")

set(VTKm_EXPORT_NAME "VTKmTargets")

# Setup default build types
include(VTKmBuildType)

# include some vtkm-specific cmake code.
include(VTKmMacros)

# Create an "interface" target library. This is not a real library but rather
# holds CMake configuration that is required for CXX targets that use VTK-m
# headers. In particular, it makes sure the appropriate C++11 version is being
# used. (The cxx_constexpr and cxx_auto_type features happen to force C++11.
# Directly asking for C++11 with this interface is not supported in CMake 3.3.)
# This is also exported so that dependent CMake projects can load the same
# configuration.
add_library(vtkm INTERFACE)
target_compile_features(vtkm INTERFACE cxx_auto_type)
target_include_directories(vtkm INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${VTKm_BINARY_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:${VTKm_INSTALL_INCLUDE_DIR}>
  )
install(TARGETS vtkm EXPORT ${VTKm_EXPORT_NAME})

# Load the base VTK-m configuration, which is required for some of the later
# config.
vtkm_configure_component_Base()
if(NOT VTKm_Base_FOUND)
  message(SEND_ERROR "Could not load base VTK-m component.")
endif()

#-----------------------------------------------------------------------------
# When using C++11 suport make sure you use the standard C++ extensions rather
# than compiler-specific versions of the extensions (to preserve portability).
set(CMAKE_CXX_EXTENSIONS False)

#-----------------------------------------------------------------------------
# Add supplemental compiler warnings, and GCC visibility support.
# include export header modules so that we can easily control symbol exporting
# VTK-m is setup by default not to export symbols unless explicitly stated.
# We prefer to only export symbols of a small set of user facing classes,
# rather than exporting all symbols. In practice we will try to not export
# symbols for any third party library
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
include(GenerateExportHeader)
include(CMake/VTKmCompilerExtras.cmake)

#-----------------------------------------------------------------------------
# Configurable Options
option(VTKm_ENABLE_CUDA "Enable Cuda support" OFF)
option(VTKm_ENABLE_TBB "Enable TBB support" OFF)
option(VTKm_ENABLE_RENDERING "Enable rendering library" ON)
option(VTKm_ENABLE_TESTING "Enable VTKm Testing" ON)
option(VTKm_ENABLE_BENCHMARKS "Enable VTKm Benchmarking" OFF)
option(VTKm_ENABLE_OSMESA "Enable creating the OSMesa canvas" OFF)

option(VTKm_ENABLE_DOCUMENTATION "Build Doxygen documentation" OFF)
option(VTKm_ENABLE_EXAMPLES "Build examples" OFF)

option(VTKm_USE_DOUBLE_PRECISION
  "Use double precision for floating point calculations"
  OFF
  )
option(VTKm_USE_64BIT_IDS "Use 64-bit indices." ON)

option(VTKm_NO_ASSERT "Disable assertions in debugging builds." OFF)
mark_as_advanced(VTKm_NO_ASSERT)

option(BUILD_SHARED_LIBS "Build VTK-m with shared libraries" ON)
set(VTKm_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

if (VTKm_ENABLE_TESTING)
  enable_testing()
  include(CTest)
  configure_file(${VTKm_SOURCE_DIR}/CTestCustom.cmake.in
    ${VTKm_BINARY_DIR}/CTestCustom.cmake @ONLY)
endif()

#-----------------------------------------------------------------------------
# Set up devices selected.
vtkm_configure_component_Serial()
if(NOT VTKm_Serial_FOUND)
  message(SEND_ERROR "Could not load serial VTK-m component.")
endif()
if (VTKm_ENABLE_TBB)
  vtkm_configure_component_TBB()
  if(NOT VTKm_TBB_FOUND)
    message(SEND_ERROR "Could not load TBB configuration. If TBB is not available, turn off VTKm_ENABLE_TBB.")
  endif()
endif (VTKm_ENABLE_TBB)
if (VTKm_ENABLE_CUDA)
  vtkm_configure_component_CUDA()
  if(NOT VTKm_CUDA_FOUND)
    message(SEND_ERROR "Could not load CUDA configuration. If CUDA is not available, turn off VTKm_ENABLE_CUDA.")
  endif()
endif (VTKm_ENABLE_CUDA)

#-----------------------------------------------------------------------------

## Set the directory where the binaries will be stored
set( VTKm_EXECUTABLE_OUTPUT_PATH
  ${PROJECT_BINARY_DIR}/bin
  CACHE PATH
  "Directory where all executable will be stored"
)

## Set the directory where the libraries will be stored
set( VTKm_LIBRARY_OUTPUT_PATH
  ${PROJECT_BINARY_DIR}/lib
  CACHE PATH
  "Directory where all the libraries will be stored"
)
mark_as_advanced(
  VTKm_EXECUTABLE_OUTPUT_PATH
  VTKm_LIBRARY_OUTPUT_PATH)

#-----------------------------------------------------------------------------
# Add "meta" tests that check the state of the repository
# SystemInformation prints out information about the current configuration
# CopyrightStatement checks that the copyright statement is in all source files
# SourceInBuild checks that all source files are listed in the build
if (VTKm_ENABLE_TESTING)
  add_test(NAME SystemInformation
    COMMAND ${CMAKE_COMMAND} "-DVTKm_SOURCE_DIR=${VTKm_SOURCE_DIR}" "-DVTKm_BINARY_DIR=${VTKm_BINARY_DIR}" -P "${VTKm_SOURCE_DIR}/CMake/VTKmSystemInformation.cmake"
    )
  add_test(NAME CopyrightStatement
    COMMAND ${CMAKE_COMMAND} "-DVTKm_SOURCE_DIR=${VTKm_SOURCE_DIR}" -P "${VTKm_SOURCE_DIR}/CMake/VTKmCheckCopyright.cmake"
    )
  add_test(NAME SourceInBuild
    COMMAND ${CMAKE_COMMAND} "-DVTKm_SOURCE_DIR=${VTKm_SOURCE_DIR}" -P "${VTKm_SOURCE_DIR}/CMake/VTKmCheckSourceInBuild.cmake"
    )
endif (VTKm_ENABLE_TESTING)

#-----------------------------------------------------------------------------
# Check basic type sizes.
include(CheckTypeSize)

check_type_size(float VTKm_SIZE_FLOAT BUILTIN_TYPES_ONLY)
check_type_size(double VTKm_SIZE_DOUBLE BUILTIN_TYPES_ONLY)
check_type_size(char VTKm_SIZE_CHAR BUILTIN_TYPES_ONLY)
check_type_size(short VTKm_SIZE_SHORT BUILTIN_TYPES_ONLY)
check_type_size(int VTKm_SIZE_INT BUILTIN_TYPES_ONLY)
check_type_size(long VTKm_SIZE_LONG BUILTIN_TYPES_ONLY)
check_type_size("long long" VTKm_SIZE_LONG_LONG BUILTIN_TYPES_ONLY)

#-----------------------------------------------------------------------------
# Find the Python interpreter, which we will use during the build process
find_package(PythonInterp)

#-----------------------------------------------------------------------------
# Find Pyexpander in case somebody wants to update the auto generated
# faux variadic template code
find_package(Pyexpander)

#-----------------------------------------------------------------------------
# Add subdirectories
add_subdirectory(vtkm)

#-----------------------------------------------------------------------------
# Build documentation
if (VTKm_ENABLE_DOCUMENTATION)
  include(CMake/VTKmBuildDocumentation.cmake)
  vtkm_build_documentation()
endif()

#-----------------------------------------------------------------------------
# Ready files for find_package
include(CMakePackageConfigHelpers)

string(REPLACE ";" " " VTKm_AVAILABLE_COMPONENTS_LIST
  "${VTKm_AVAILABLE_COMPONENTS}"
  )

set(VTKm_LOAD_COMPONENTS_COMMANDS "# Clear out old FOUND flags")
foreach(component ${VTKm_AVAILABLE_COMPONENTS})
  set(VTKm_LOAD_COMPONENTS_COMMANDS "${VTKm_LOAD_COMPONENTS_COMMANDS}
set(VTKm_${component}_FOUND)"
    )
endforeach(component)

set(VTKm_LOAD_COMPONENTS_COMMANDS "${VTKm_LOAD_COMPONENTS_COMMANDS}

# Load each component selected in find_package
foreach(comp \${VTKm_FIND_COMPONENTS})")

foreach(component ${VTKm_AVAILABLE_COMPONENTS})
  set(VTKm_LOAD_COMPONENTS_COMMANDS "${VTKm_LOAD_COMPONENTS_COMMANDS}
  if(\"${component}\" STREQUAL \${comp})
    vtkm_configure_component_${component}()
    if(VTKm_${component}_FOUND AND NOT VTKm_CONFIGURE_QUIET)
      message(STATUS \"Successfully loaded VTK-m component ${component}\")
    else()
      if(VTKm_FIND_REQUIRED_${component} AND NOT VTKm_CONFIGURE_QUIET)
        message(\"Could not load REQUIRED VTK-m component ${component}\")
      elseif(NOT VTKm_CONFIGURE_QUIET)
        message(STATUS \"Could not load optional VTK-m component ${component}\")
      endif()
    endif()
    continue()
  endif()"
    )
endforeach()

set(VTKm_LOAD_COMPONENTS_COMMANDS "${VTKm_LOAD_COMPONENTS_COMMANDS}
  if(NOT VTKm_CONFIGURE_QUIET)
    message(WARNING \"No such VTK-m component: \${comp}\")
  endif()
endforeach()" )

configure_package_config_file(
  ${VTKm_SOURCE_DIR}/CMake/VTKmConfig.cmake.in
  ${VTKm_BINARY_DIR}/${VTKm_INSTALL_CONFIG_DIR}/VTKmConfig.cmake
  INSTALL_DESTINATION ${VTKm_INSTALL_CONFIG_DIR}
  PATH_VARS
    VTKm_INSTALL_INCLUDE_DIR
    VTKm_INSTALL_CONFIG_DIR
    VTKm_INSTALL_LIB_DIR
    VTKm_INSTALL_BIN_DIR
    VTKm_INSTALL_CMAKE_MODULE_DIR
  )

write_basic_package_version_file(
  ${VTKm_BINARY_DIR}/${VTKm_INSTALL_CONFIG_DIR}/VTKmConfigVersion.cmake
  VERSION ${VTKm_VERSION}
  COMPATIBILITY ExactVersion )
install(
  FILES
    ${VTKm_BINARY_DIR}/${VTKm_INSTALL_CONFIG_DIR}/VTKmConfig.cmake
    ${VTKm_BINARY_DIR}/${VTKm_INSTALL_CONFIG_DIR}/VTKmConfigVersion.cmake
  DESTINATION ${VTKm_INSTALL_CONFIG_DIR}
  )

# Install the readme and license files.
install(FILES ${VTKm_SOURCE_DIR}/README.md
  DESTINATION ${VTKm_INSTALL_CONFIG_DIR}
  RENAME VTKmREADME.md
  )
install(FILES ${VTKm_SOURCE_DIR}/LICENSE.txt
  DESTINATION ${VTKm_INSTALL_CONFIG_DIR}
  RENAME VTKmLICENSE.txt
  )

# Install helper configure files.
install(
  FILES
    ${VTKm_SOURCE_DIR}/CMake/FindEGL.cmake
    ${VTKm_SOURCE_DIR}/CMake/FindGLEW.cmake
    ${VTKm_SOURCE_DIR}/CMake/FindGLFW.cmake
    ${VTKm_SOURCE_DIR}/CMake/FindMESA.cmake
    ${VTKm_SOURCE_DIR}/CMake/FindTBB.cmake
    ${VTKm_SOURCE_DIR}/CMake/FindThrust.cmake
  DESTINATION ${VTKm_INSTALL_CMAKE_MODULE_DIR}
  )

# Install support files.
install(
  FILES
    ${VTKm_SOURCE_DIR}/CMake/VTKmMacros.cmake
    ${VTKm_SOURCE_DIR}/CMake/VTKmConfigureComponents.cmake
    ${VTKm_SOURCE_DIR}/CMake/VTKmCompilerOptimizations.cmake
    ${VTKm_SOURCE_DIR}/CMake/VTKmDetectCUDAVersion.cu
    ${VTKm_SOURCE_DIR}/CMake/VTKmExportHeaderTemplate.h.in
  DESTINATION ${VTKm_INSTALL_CMAKE_MODULE_DIR}
  )

# Create and install exports for external projects
export(EXPORT ${VTKm_EXPORT_NAME}
  FILE ${VTKm_BINARY_DIR}/${VTKm_INSTALL_CONFIG_DIR}/VTKmTargets.cmake
  )
install(EXPORT ${VTKm_EXPORT_NAME}
  DESTINATION ${VTKm_INSTALL_CONFIG_DIR}
  FILE VTKmTargets.cmake
  )

# Enable CPack packaging
set(CPACK_PACKAGE_DESCRIPTION_FILE ${VTKm_SOURCE_DIR}/README.md)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The VTKm Toolkit")
set(CPACK_PACKAGE_NAME "VTKm")
set(CPACK_PACKAGE_VERSION_MAJOR ${VTKm_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VTKm_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VTKm_VERSION_PATCH})
set(CPACK_PACKAGE_FILE_NAME "VTKm-${VTKm_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE ${VTKm_SOURCE_DIR}/LICENSE.txt)
set(CPACK_RESOURCE_FILE_README ${VTKm_SOURCE_DIR}/README.md)
include(CPack)

#-----------------------------------------------------------------------------
# Build examples
if(VTKm_ENABLE_EXAMPLES)
  add_subdirectory(examples)
endif(VTKm_ENABLE_EXAMPLES)
