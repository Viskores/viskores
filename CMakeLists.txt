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

# We require CMake 3.3 for Modern CMake which as features such as:
#   - Support for target_sources
#   - Support for usage requirements
#
# If you want CUDA support, you will need to have CMake 3.9 on Linux/OSX.
# We require CMake 3.11 with the MSVC generator as the $<COMPILE_LANGUAGE:>
# generator expression is not supported on older versions.
cmake_minimum_required(VERSION 3.3...3.12)
project (VTKm)

if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
  cmake_minimum_required(VERSION 3.11...3.12 FATAL_ERROR)
endif()

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
if (NOT DEFINED VTKm_INSTALL_SHARE_DIR)
  set(VTKm_INSTALL_SHARE_DIR "share/vtkm-${VTKm_VERSION_MAJOR}.${VTKm_VERSION_MINOR}")
endif()
if (NOT DEFINED VTKm_INSTALL_CMAKE_MODULE_DIR)
  set(VTKm_INSTALL_CMAKE_MODULE_DIR "${VTKm_INSTALL_SHARE_DIR}/cmake")
endif()
if (NOT DEFINED VTKm_BUILD_CMAKE_BASE_DIR)
  set(VTKm_BUILD_CMAKE_BASE_DIR "${VTKm_BINARY_DIR}")
endif ()

set(VTKm_BINARY_INCLUDE_DIR "${VTKm_BINARY_DIR}/include")

if (NOT DEFINED VTKm_EXPORT_NAME)
  set(VTKm_EXPORT_NAME "VTKmTargets")
endif()

#-----------------------------------------------------------------------------
# vtkm_option(variable doc [initial])
#   Provides an option if it is not already defined.
macro (vtkm_option variable)
  if (NOT DEFINED "${variable}")
    option("${variable}" ${ARGN})
  endif ()
endmacro ()

# Configurable Options
vtkm_option(VTKm_ENABLE_CUDA "Enable Cuda support" OFF)
vtkm_option(VTKm_ENABLE_TBB "Enable TBB support" OFF)
vtkm_option(VTKm_ENABLE_OPENMP "Enable OpenMP support" OFF)
vtkm_option(VTKm_ENABLE_RENDERING "Enable rendering library" ON)
vtkm_option(VTKm_ENABLE_TESTING "Enable VTKm Testing" ON)
vtkm_option(VTKm_ENABLE_BENCHMARKS "Enable VTKm Benchmarking" OFF)
vtkm_option(VTKm_ENABLE_MPI "Enable MPI support" OFF)

vtkm_option(VTKm_ENABLE_DOCUMENTATION "Build Doxygen documentation" OFF)
vtkm_option(VTKm_ENABLE_EXAMPLES "Build examples" OFF)

vtkm_option(VTKm_USE_DOUBLE_PRECISION "Use double precision for floating point calculations" OFF)
vtkm_option(VTKm_USE_64BIT_IDS "Use 64-bit indices." ON)

# When VTK-m is embedded into larger projects they may desire to turn off
# VTK-m internal assert checks when in debug mode to improve debug runtime
# performance.
vtkm_option(VTKm_NO_ASSERT "Disable assertions in debugging builds." OFF)

# When VTK-m is embedded into larger projects that wish to make end user
# applications they want to only install libraries and don't want CMake/headers
# installed.
vtkm_option(VTKm_INSTALL_ONLY_LIBRARIES "install only vtk-m libraries and no headers" OFF)

# VTK-m is setup by default not to export symbols unless explicitly stated.
# We prefer to only export symbols of a small set of user facing classes,
# rather than exporting all symbols. This flag is added so that consumers
# which require static builds can force all symbols on, which is something
# VTK does.
vtkm_option(VTKm_USE_DEFAULT_SYMBOL_VISIBILITY "Don't explicitly hide symbols from libraries." OFF)

vtkm_option(BUILD_SHARED_LIBS "Build VTK-m with shared libraries" ON)
set(VTKm_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

# This flag can be used to prevent VTK-m from exporting its warning flags in its
# build interface. This is useful when building VTK-m as a thirdparty library
# and the warnings are too strict for the parent project.
vtkm_option(VTKm_ENABLE_DEVELOPER_FLAGS "Enable compiler flags that are useful while developing VTK-m" ON)

mark_as_advanced(
  VTKm_NO_ASSERT
  VTKm_INSTALL_ONLY_LIBRARIES
  VTKm_USE_DEFAULT_SYMBOL_VISIBILITY
  VTKm_ENABLE_DEVELOPER_FLAGS
  )

#-----------------------------------------------------------------------------
# When using C++11 support make sure you use the standard C++ extensions rather
# than compiler-specific versions of the extensions (to preserve portability).
set(CMAKE_CXX_EXTENSIONS False)

# Setup default build types
include(VTKmBuildType)

# Include the vtk-m wrappers
include(VTKmWrappers)

# Create vtkm_compiler_flags library. This is an interface library that
# holds all the C++ compiler flags that are needed for consumers and
# when building VTK-m.
include(VTKmCompilerFlags)


#-----------------------------------------------------------------------------
# When building VTK-m it self we want to explicitly disable compiler extensions
set(CMAKE_CXX_EXTENSIONS OFF)

#-----------------------------------------------------------------------------

## Set the directory where the binaries will be stored
if(NOT VTKm_EXECUTABLE_OUTPUT_PATH)
  set(VTKm_EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
endif()

## Set the directory where the libraries will be stored
if(NOT VTKm_LIBRARY_OUTPUT_PATH)
  set(VTKm_LIBRARY_OUTPUT_PATH  ${PROJECT_BINARY_DIR}/lib)
endif()

#-----------------------------------------------------------------------------
if (VTKm_ENABLE_TESTING)
  enable_testing()
  # Only include CTest if it has not been included by a superproject. The
  # variable DEFAULT_CTEST_CONFIGURATION_TYPE is a non-cached variable set by
  # CTest.cmake, so we'll use that to determine if it's already included.
  if(NOT DEFINED DEFAULT_CTEST_CONFIGURATION_TYPE)
    include(CTest)
    # Mark this as advanced to avoid confusion, since we actually rely on
    # VTKm_ENABLE_TESTING.
    mark_as_advanced(BUILD_TESTING)
  endif()

  configure_file(${VTKm_SOURCE_DIR}/CTestCustom.cmake.in
    ${VTKm_BINARY_DIR}/CTestCustom.cmake @ONLY)

  #-----------------------------------------------------------------------------
  # Add "meta" tests that check the state of the repository
  # SystemInformation prints out information about the current configuration
  # CopyrightStatement checks that the copyright statement is in all source files
  # SourceInBuild checks that all source files are listed in the build
  add_test(NAME SystemInformation
    COMMAND ${CMAKE_COMMAND} "-DVTKm_SOURCE_DIR=${VTKm_SOURCE_DIR}" "-DVTKm_BINARY_DIR=${VTKm_BINARY_DIR}" -P "${VTKm_SOURCE_DIR}/CMake/VTKmSystemInformation.cmake"
    )
  add_test(NAME CopyrightStatement
    COMMAND ${CMAKE_COMMAND} "-DVTKm_SOURCE_DIR=${VTKm_SOURCE_DIR}" -P "${VTKm_SOURCE_DIR}/CMake/VTKmCheckCopyright.cmake"
    )
  # increase timeout since on some machines CopyrightStatement test takes a long time.
  set_tests_properties(CopyrightStatement PROPERTIES TIMEOUT 300)

  add_test(NAME SourceInBuild
    COMMAND ${CMAKE_COMMAND} "-DVTKm_SOURCE_DIR=${VTKm_SOURCE_DIR}" -P "${VTKm_SOURCE_DIR}/CMake/VTKmCheckSourceInBuild.cmake"
    )

  #-----------------------------------------------------------------------------
  # Find the Python interpreter, which we will use during the build process
  find_package(PythonInterp QUIET)

  #-----------------------------------------------------------------------------
  # Find Pyexpander in case somebody wants to update the auto generated
  # faux variadic template code
  find_package(Pyexpander QUIET)
endif (VTKm_ENABLE_TESTING)

#-----------------------------------------------------------------------------
# Check basic type sizes.
include(CheckTypeSize)

check_type_size(long VTKm_SIZE_LONG BUILTIN_TYPES_ONLY)
check_type_size("long long" VTKm_SIZE_LONG_LONG BUILTIN_TYPES_ONLY)

#-----------------------------------------------------------------------------
# Add subdirectories
if(VTKm_ENABLE_MPI)
  # This `if` is temporary and will be removed once `diy` supports building
  # without MPI.
  if (NOT MPI_C_FOUND)
    find_package(MPI ${VTKm_FIND_PACKAGE_QUIETLY})
  endif()
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
  ${VTKm_BUILD_CMAKE_BASE_DIR}/${VTKm_INSTALL_CONFIG_DIR}/VTKmConfig.cmake
  INSTALL_DESTINATION ${VTKm_INSTALL_CONFIG_DIR}
  PATH_VARS
    VTKm_INSTALL_INCLUDE_DIR
    VTKm_INSTALL_CONFIG_DIR
    VTKm_INSTALL_LIB_DIR
    VTKm_INSTALL_BIN_DIR
    VTKm_INSTALL_CMAKE_MODULE_DIR
  )

write_basic_package_version_file(
  ${VTKm_BUILD_CMAKE_BASE_DIR}/${VTKm_INSTALL_CONFIG_DIR}/VTKmConfigVersion.cmake
  VERSION ${VTKm_VERSION}
  COMPATIBILITY ExactVersion )

if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
  install(
    FILES
      ${VTKm_BUILD_CMAKE_BASE_DIR}/${VTKm_INSTALL_CONFIG_DIR}/VTKmConfig.cmake
      ${VTKm_BUILD_CMAKE_BASE_DIR}/${VTKm_INSTALL_CONFIG_DIR}/VTKmConfigVersion.cmake
    DESTINATION ${VTKm_INSTALL_CONFIG_DIR}
    )

  # Install the readme and license files.
  install(FILES ${VTKm_SOURCE_DIR}/README.md
    DESTINATION ${VTKm_INSTALL_SHARE_DIR}
    RENAME VTKmREADME.md
    )
  install(FILES ${VTKm_SOURCE_DIR}/LICENSE.txt
    DESTINATION ${VTKm_INSTALL_SHARE_DIR}
    RENAME VTKmLICENSE.txt
    )

  # Install helper configure files.
  install(
    FILES
      ${VTKm_SOURCE_DIR}/CMake/FindTBB.cmake
      ${VTKm_SOURCE_DIR}/CMake/FindOpenGL.cmake
      ${VTKm_SOURCE_DIR}/CMake/FindOpenMP.cmake
    DESTINATION ${VTKm_INSTALL_CMAKE_MODULE_DIR}
    )

  # Install support files.
  install(
    FILES
      ${VTKm_SOURCE_DIR}/CMake/VTKmCPUVectorization.cmake
      ${VTKm_SOURCE_DIR}/CMake/VTKmDetectCUDAVersion.cu
      ${VTKm_SOURCE_DIR}/CMake/VTKmDeviceAdapters.cmake
      ${VTKm_SOURCE_DIR}/CMake/VTKmExportHeaderTemplate.h.in
      ${VTKm_SOURCE_DIR}/CMake/VTKmRenderingContexts.cmake
      ${VTKm_SOURCE_DIR}/CMake/VTKmWrappers.cmake
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
endif()

vtkm_option(VTKm_ENABLE_CPACK "Enable CPack packaging of VTKm" ON)
if (VTKm_ENABLE_CPACK)
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
endif ()

#-----------------------------------------------------------------------------
#add the benchmarking folder
if(VTKm_ENABLE_BENCHMARKS)
    add_subdirectory(benchmarking)
endif()

#-----------------------------------------------------------------------------
# Build examples
if(VTKm_ENABLE_EXAMPLES)
  add_subdirectory(examples)
endif(VTKm_ENABLE_EXAMPLES)
