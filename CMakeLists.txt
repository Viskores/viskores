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

# We require CMake 3.9 for Modern CMake which as features such as:
#   - Support for OBJECTs in target_sources
#   - Support for meta level compiler features
#   - Proper if() argument expansion
#   - Proper CUDA support
#   - Support for usage requirements
#
# We require CMake 3.9 for CUDA support
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

if (NOT DEFINED VTKm_EXPORT_NAME)
  set(VTKm_EXPORT_NAME "VTKmTargets")
endif()


set(VTKm_EXPORT_NAME "VTKmTargets")

#-----------------------------------------------------------------------------
# Configurable Options
option(VTKm_ENABLE_CUDA "Enable Cuda support" OFF)
option(VTKm_ENABLE_TBB "Enable TBB support" OFF)
option(VTKm_ENABLE_RENDERING "Enable rendering library" ON)
option(VTKm_ENABLE_TESTING "Enable VTKm Testing" ON)
option(VTKm_ENABLE_BENCHMARKS "Enable VTKm Benchmarking" OFF)
option(VTKm_ENABLE_MPI "Enable MPI support" OFF)

option(VTKm_ENABLE_DOCUMENTATION "Build Doxygen documentation" OFF)
option(VTKm_ENABLE_EXAMPLES "Build examples" OFF)

option(VTKm_USE_DOUBLE_PRECISION "Use double precision for floating point calculations" OFF)
option(VTKm_USE_64BIT_IDS "Use 64-bit indices." ON)

option(VTKm_NO_ASSERT "Disable assertions in debugging builds." OFF)
mark_as_advanced(VTKm_NO_ASSERT)

# VTK-m is setup by default not to export symbols unless explicitly stated.
# We prefer to only export symbols of a small set of user facing classes,
# rather than exporting all symbols. This flag is added so that consumers
# which require static builds can force all symbols on, which is something
# VTK does.
option(VTKm_USE_DEFAULT_SYMBOL_VISIBILITY "Don't explicitly hide symbols from libraries." OFF)
mark_as_advanced(VTKm_USE_DEFAULT_SYMBOL_VISIBILITY)

option(BUILD_SHARED_LIBS "Build VTK-m with shared libraries" ON)
set(VTKm_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

# Setup default build types
include(VTKmBuildType)

# Determine VTK-m version
include(Utilities/Git/Git.cmake)
include(VTKmDetermineVersion)

# Load hardcoded version in case this is not a Git repository
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "VTKm")
# Get the version from git if we can
determine_version(${VTKm_SOURCE_DIR} ${GIT_EXECUTABLE} "VTKm")

# Create vtkm_compiler_flags library. This is an interface library that
# holds all the C++ compiler flags that are needed for consumers and
# when building VTK-m.
include(VTKmCompilerFlags)
include(VTKmWrappers)
include(VTKmCPUVectorization)

#-----------------------------------------------------------------------------
# When building VTK-m it self we want to explicitly disable compiler extensions
set(CMAKE_CXX_EXTENSIONS OFF)

if (VTKm_ENABLE_TESTING)
  enable_testing()
  include(CTest)
  configure_file(${VTKm_SOURCE_DIR}/CTestCustom.cmake.in
    ${VTKm_BINARY_DIR}/CTestCustom.cmake @ONLY)
endif()

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
if(VTKm_ENABLE_MPI)
  # This `if` is temporary and will be removed once `diy` supports building
  # without MPI.
  add_subdirectory(diy)
endif()
add_subdirectory(vtkm)

#-----------------------------------------------------------------------------
# Build documentation
if (VTKm_ENABLE_DOCUMENTATION)
  include(CMake/VTKmBuildDocumentation.cmake)
endif()

#-----------------------------------------------------------------------------
# Ready files for find_package
include(CMakePackageConfigHelpers)

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
    ${VTKm_SOURCE_DIR}/CMake/FindTBB.cmake
    ${VTKm_SOURCE_DIR}/CMake/FindOpenGL.cmake
  DESTINATION ${VTKm_INSTALL_CMAKE_MODULE_DIR}
  )

# Install support files.
install(
  FILES
    ${VTKm_SOURCE_DIR}/CMake/VTKmWrappers.cmake
    ${VTKm_SOURCE_DIR}/CMake/VTKmBackends.cmake
    ${VTKm_SOURCE_DIR}/CMake/VTKmRenderingContexts.cmake
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
