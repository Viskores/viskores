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

cmake_minimum_required(VERSION 2.8.11)

#setup policy rules for CMake 3.0 while we have a minimum required of 2.8.X
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)#Clang and AppleClang are different compiler ids
endif()

#setup policy rules for CMake 3.1 while we have a minimum required of 2.8.X
if(POLICY CMP0054)
  cmake_policy(SET CMP0053 NEW)#Enable faster parser engine
  cmake_policy(SET CMP0054 NEW)#simplify if() argument expansion
endif()

#setup policy rules for CMake 3.3 while we have a minimum required of 2.8.X
if(POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)#Honor visibility properties for all targets
endif()

project (VTKm)

set(VTKm_MAJOR_VERSION 0)
set(VTKm_MINOR_VERSION 1)
set(VTKm_PATCH_VERSION 0)
set(VTKm_VERSION "${VTKm_MAJOR_VERSION}.${VTKm_MINOR_VERSION}.${VTKm_PATCH_VERSION}")

set(VTKm_INSTALL_INCLUDE_DIR "include")
set(VTKm_INSTALL_CONFIG_DIR "include")
set(VTKm_INSTALL_CMAKE_MODULE_DIR "share/vtkm/cmake")

set(VTKm_REQUIRED_BOOST_VERSION "1.48")

# include some vtkm-specific cmake code.
include(CMake/VTKmMacros.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VTKm_SOURCE_DIR}/CMake)

#-----------------------------------------------------------------------------
# List of Boost features used:
# * Smart Ptr
# * Meta programming language
# We check for boost first, as the device configuration code requires boost
# to be found
find_package(BoostHeaders ${VTKm_REQUIRED_BOOST_VERSION} REQUIRED)
if(NOT Boost_FOUND)
  message(FATAL_ERROR "Please specify where to find boost headers (${VTKm_REQUIRED_BOOST_VERSION}+)" )
endif()

#-----------------------------------------------------------------------------
# Check for Cxx11 support.
option(VTKm_FORCE_ANSI
 "Turn off compiling any features not compatible with ISO-C++98 (ANSI)." OFF)
if (NOT VTKm_FORCE_ANSI)
  include(CMake/CheckCXX11Features.cmake)
else (NOT VTKm_FORCE_ANSI)
  set(VTKm_NO_VARIADIC_TEMPLATE TRUE)
endif ()

#-----------------------------------------------------------------------------
# Add supplemental compiler warnings, and GCC visibility support.
include(CMake/VTKmCompilerExtras.cmake)

#-----------------------------------------------------------------------------
# Configurable Options
option(VTKm_ENABLE_CUDA "Enable Cuda support" OFF)
option(VTKm_ENABLE_TBB "Enable TBB support" OFF)
option(VTKm_ENABLE_TESTING "Enable VTKm Testing" ON)
option(VTKm_ENABLE_BENCHMARKS "Enable VTKm Benchmarking" OFF)

option(VTKm_BUILD_DOCUMENTATION "Build Doxygen documentation" OFF)
option(VTKm_BUILD_EXAMPLES "Build examples" OFF)

option(VTKm_USE_DOUBLE_PRECISION
  "Use double precision for floating point calculations"
  OFF
  )
option(VTKm_USE_64BIT_IDS "Use 64-bit indices." ON)

if (VTKm_ENABLE_TESTING)
  enable_testing()
  include(CTest)
endif()

#-----------------------------------------------------------------------------
# Set up devices selected.
vtkm_configure_device(Serial)
if (VTKm_ENABLE_CUDA)
  vtkm_configure_device(Cuda)
endif (VTKm_ENABLE_CUDA)
if (VTKm_ENABLE_TBB)
  vtkm_configure_device(TBB)
endif (VTKm_ENABLE_TBB)

#-----------------------------------------------------------------------------

## Set the directory where the binaries will be stored
set( EXECUTABLE_OUTPUT_PATH
  ${PROJECT_BINARY_DIR}/bin
  CACHE PATH
  "Directory where all executable will be stored"
)

## Set the directory where the libraries will be stored
set( LIBRARY_OUTPUT_PATH
  ${PROJECT_BINARY_DIR}/libs
  CACHE PATH
  "Directory where all the libraries will be stored"
)
mark_as_advanced(
  EXECUTABLE_OUTPUT_PATH
  LIBRARY_OUTPUT_PATH)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

#-----------------------------------------------------------------------------
# Add test to check for copyright statement on all source files.
# Also add test to output system information.
if (VTKm_ENABLE_TESTING)
  add_test(NAME CopyrightStatement
    COMMAND ${CMAKE_COMMAND} "-DVTKm_SOURCE_DIR=${VTKm_SOURCE_DIR}" -P "${VTKm_SOURCE_DIR}/CMake/VTKmCheckCopyright.cmake"
    )
  add_test(NAME SystemInformation
    COMMAND ${CMAKE_COMMAND} "-DVTKm_SOURCE_DIR=${VTKm_SOURCE_DIR}" "-DVTKm_BINARY_DIR=${VTKm_BINARY_DIR}" -P "${VTKm_SOURCE_DIR}/CMake/VTKmSystemInformation.cmake"
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
# Find OpenGL and GLEW, if both are found we can enable
# the OpenGL Interop support. We use
include(CMakeDependentOption)

# enable Interop only if we have OpenGL and GLEW
find_package(OpenGL)
find_package(GLEW)
find_package(GLUT)

#dependent option reads, value to set, if condition is true, otherwise
#use last value
CMAKE_DEPENDENT_OPTION(VTKm_ENABLE_OPENGL_INTEROP
                       "Enable OpenGL Interop will require GLEW"
                       ON "OPENGL_FOUND;GLEW_FOUND" OFF)

#Only enable OpenGL Interop tests if we have Interop enabled
#and we have GLUT
#dependent option reads, value to set, if condition is true, otherwise
#use last value
CMAKE_DEPENDENT_OPTION(VTKm_ENABLE_OPENGL_TESTS
                       "Enable OpenGL Interop Render Window Tests"
                       ON "VTKm_ENABLE_OPENGL_INTEROP;GLUT_FOUND" OFF)

#on unix/linux Glew uses pthreads, so we need to find that, and link to it
#explicitly or else in release mode we get sigsegv on launch
if(UNIX AND VTKm_ENABLE_OPENGL_INTEROP)
  find_package(Threads REQUIRED)
endif()

#-----------------------------------------------------------------------------
# Find Pyexpander in case somebody wants to update the auto generated
# faux variadic template code
find_package(Pyexpander)

#-----------------------------------------------------------------------------
# Add subdirectories
add_subdirectory(vtkm)

#-----------------------------------------------------------------------------
# Build documentation
if (VTKm_BUILD_DOCUMENTATION)
  include(CMake/VTKmBuildDocumentation.cmake)
  vtkm_build_documentation()
endif()

#-----------------------------------------------------------------------------
# Build examples
if(VTKm_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif(VTKm_BUILD_EXAMPLES)

#-----------------------------------------------------------------------------
# Build the configure file.
# need to set numerous VTKm cmake properties to the naming convention
# that we exepect for our C++ defines.

set(VTKM_USE_DOUBLE_PRECISION ${VTKm_USE_DOUBLE_PRECISION})
set(VTKM_USE_64BIT_IDS ${VTKm_USE_64BIT_IDS})

set(VTKM_ENABLE_CUDA ${VTKm_ENABLE_CUDA})
set(VTKM_ENABLE_TBB ${VTKm_ENABLE_TBB})

set(VTKM_ENABLE_OPENGL_INTEROP ${VTKm_ENABLE_OPENGL_INTEROP})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkm/internal/Configure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkm/internal/Configure.h
  @ONLY)
vtkm_install_headers(
  vtkm/internal ${CMAKE_CURRENT_BINARY_DIR}/vtkm/internal/Configure.h)

unset(VTKM_ENABLE_OPENGL_INTEROP)

unset(VTKM_ENABLE_TBB)
unset(VTKM_ENABLE_CUDA)

unset(VTKM_USE_64BIT_IDS)
unset(VTKM_USE_DOUBLE_PRECISION)

#-----------------------------------------------------------------------------
# Configuration for build directory.
set(VTKm_INCLUDE_DIRS_CONFIG "${VTKm_SOURCE_DIR};${VTKm_BINARY_DIR}")
set(VTKm_CMAKE_MODULE_PATH_CONFIG "${VTKm_SOURCE_DIR}/CMake")
configure_file(
  ${VTKm_SOURCE_DIR}/CMake/VTKmConfig.cmake.in
  ${VTKm_BINARY_DIR}/VTKmConfig.cmake
  @ONLY IMMEDIATE
  )

# Configuration for install.
set(VTKm_INCLUDE_DIRS_CONFIG "\${_install_dir}/${VTKm_INSTALL_INCLUDE_DIR}")
set(VTKm_CMAKE_MODULE_PATH_CONFIG "\${_install_dir}/${VTKm_INSTALL_CMAKE_MODULE_DIR}")
configure_file(
  ${VTKm_SOURCE_DIR}/CMake/VTKmConfig.cmake.in
  ${VTKm_BINARY_DIR}/VTKmConfig.cmake.install
  @ONLY IMMEDIATE
  )
install(FILES ${VTKm_BINARY_DIR}/VTKmConfig.cmake.install
  DESTINATION ${VTKm_INSTALL_CONFIG_DIR}
  RENAME VTKmConfig.cmake
  )

# Create supplemental version configuration file.
configure_file(
  ${VTKm_SOURCE_DIR}/CMake/VTKmConfigVersion.cmake.in
  ${VTKm_BINARY_DIR}/VTKmConfigVersion.cmake
  @ONLY
  )
install(FILES ${VTKm_BINARY_DIR}/VTKmConfigVersion.cmake
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
    ${VTKm_SOURCE_DIR}/CMake/FindBoostHeaders.cmake
    ${VTKm_SOURCE_DIR}/CMake/FindGLEW.cmake
    ${VTKm_SOURCE_DIR}/CMake/FindTBB.cmake
    ${VTKm_SOURCE_DIR}/CMake/FindThrust.cmake
  DESTINATION ${VTKm_INSTALL_CMAKE_MODULE_DIR}
  )

# Install Use files.
install(
  FILES
    ${VTKm_SOURCE_DIR}/CMake/UseVTKmSerial.cmake
    ${VTKm_SOURCE_DIR}/CMake/UseVTKmTBB.cmake
    ${VTKm_SOURCE_DIR}/CMake/UseVTKmCuda.cmake
  DESTINATION ${VTKm_INSTALL_CMAKE_MODULE_DIR}
  )


# Enable CPack packaging
set(CPACK_PACKAGE_DESCRIPTION_FILE ${VTKm_SOURCE_DIR}/README.md)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The VTKm Toolkit")
set(CPACK_PACKAGE_NAME "VTKm")
set(CPACK_PACKAGE_VERSION_MAJOR ${VTKm_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${VTKm_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${VTKm_PATCH_VERSION})
set(CPACK_PACKAGE_FILE_NAME "VTKm-${VTKm_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE ${VTKm_SOURCE_DIR}/LICENSE.txt)
set(CPACK_RESOURCE_FILE_README ${VTKm_SOURCE_DIR}/README.md)
include(CPack)
