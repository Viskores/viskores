##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

# If you want CUDA support, you will need to have CMake 3.13 on Linux/OSX.
cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project (VTKm)

# We only allow c++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# When using C++14 support make sure you use the standard C++ extensions rather
# than compiler-specific versions of the extensions (to preserve portability).
set(CMAKE_CXX_EXTENSIONS OFF)

# Update module path
set(VTKm_CMAKE_MODULE_PATH ${VTKm_SOURCE_DIR}/CMake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VTKm_CMAKE_MODULE_PATH})

# While disabled system-wide, VTK-m uses UNITY builds in some modules
set(CMAKE_UNITY_BUILD OFF)

# By default effectively disable unity builds
if (NOT DEFINED CMAKE_UNITY_BUILD_BATCH_SIZE)
  set(CMAKE_UNITY_BUILD_BATCH_SIZE 1)
endif()

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
endif()
if(NOT DEFINED VTKm_EXECUTABLE_OUTPUT_PATH)
  ## Set the directory where the binaries will be stored
  set(VTKm_EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
endif()
if(NOT DEFINED VTKm_LIBRARY_OUTPUT_PATH)
  ## Set the directory where the libraries will be stored
  set(VTKm_LIBRARY_OUTPUT_PATH  ${PROJECT_BINARY_DIR}/lib)
endif()
if (NOT DEFINED VTKm_EXPORT_NAME)
  set(VTKm_EXPORT_NAME "VTKmTargets")
endif()

set(VTKm_BINARY_INCLUDE_DIR "${VTKm_BINARY_DIR}/include")

#-----------------------------------------------------------------------------
# vtkm_option(variable doc [initial])
#   Provides an option if it is not already defined.
# This can be replaced when CMake 3.13 is our cmake_minimum_required
macro (vtkm_option variable)
  if (NOT DEFINED "${variable}")
    option("${variable}" ${ARGN})
  endif ()
endmacro ()

# Configurable Options
vtkm_option(VTKm_ENABLE_CUDA "Enable Cuda support" OFF)
vtkm_option(VTKm_ENABLE_KOKKOS "Enable Kokkos support" OFF)
vtkm_option(VTKm_ENABLE_OPENMP "Enable OpenMP support" OFF)
vtkm_option(VTKm_ENABLE_TBB "Enable TBB support" OFF)
vtkm_option(VTKm_ENABLE_RENDERING "Enable rendering library" ON)
vtkm_option(VTKm_ENABLE_BENCHMARKS "Enable VTKm Benchmarking" OFF)
vtkm_option(VTKm_ENABLE_MPI "Enable MPI support" OFF)
vtkm_option(VTKm_ENABLE_DOCUMENTATION "Build Doxygen documentation" OFF)
vtkm_option(VTKm_ENABLE_EXAMPLES "Build examples" OFF)
vtkm_option(VTKm_ENABLE_TUTORIALS "Build tutorials" OFF)
if (NOT DEFINED VTKm_ENABLE_TESTING)
    if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
      vtkm_option(VTKm_ENABLE_TESTING "Enable VTKm Testing" ON)
    else()
      vtkm_option(VTKm_ENABLE_TESTING "Enable VTKm Testing" OFF)
    endif()
endif()

# By default: (VTKm_ENABLE_TESTING OR VTKm_ENABLE_BENCHMARKS) -> VTKm_ENABLE_TESTING_LIBRARY
include(CMakeDependentOption)
cmake_dependent_option(VTKm_ENABLE_TESTING_LIBRARY "Enable VTKm Testing Library"
  OFF "NOT VTKm_ENABLE_TESTING;NOT VTKm_ENABLE_BENCHMARKS" ON)
mark_as_advanced(VTKm_ENABLE_TESTING_LIBRARY)

# We may want to make finer controls on whether libraries/modules get built.
# VTK uses the concept of groups for its modules
vtkm_option(VTKm_BUILD_ALL_LIBRARIES
  "Build all libraries by default. (Can be overridden for each library.)"
  ON
  )
mark_as_advanced(VTKm_BUILD_ALL_LIBRARIES)

vtkm_option(VTKm_USE_DOUBLE_PRECISION "Use double precision for floating point calculations" OFF)
vtkm_option(VTKm_USE_64BIT_IDS "Use 64-bit indices." ON)

vtkm_option(VTKm_ENABLE_HDF5_IO "Enable HDF5 support" OFF)
if (VTKm_ENABLE_HDF5_IO)
  find_package(HDF5 REQUIRED COMPONENTS HL)
endif()

# VTK-m will turn on logging by default, but will set the default
# logging level to WARN.  This option should not be visible by default
# in the GUI, as ERROR and WARN level logging should not interfere
# with the performance of vtk-m
vtkm_option(VTKm_ENABLE_LOGGING "Enable VTKm Logging" ON)

# When VTK-m is embedded into larger projects they may desire to turn off
# VTK-m internal assert checks when in debug mode to improve debug runtime
# performance.
vtkm_option(VTKm_NO_ASSERT "Disable assertions in debugging builds." OFF)

# The CUDA compiler (as of CUDA 11) takes a surprising long time to compile
# kernels with assert in them. By default we turn off asserts when compiling
# for CUDA devices.
vtkm_option(VTKm_NO_ASSERT_CUDA "Disable assertions for CUDA devices." ON)

# The HIP compiler (as of ROCm 3.7) takes a surprising long time to compile
# kernels with assert in them they generate `printf` calls which are very
# slow ( cause massive register spillage). By default we turn off asserts when
# compiling for HIP devices.
vtkm_option(VTKm_NO_ASSERT_HIP "Disable assertions for HIP devices." ON)

# When VTK-m is embedded into larger projects that wish to make end user
# applications they want to only install libraries and don't want CMake/headers
# installed.
vtkm_option(VTKm_INSTALL_ONLY_LIBRARIES "install only vtk-m libraries and no headers" OFF)

# Install examples projects
cmake_dependent_option(VTKm_INSTALL_EXAMPLES "Install examples" OFF "NOT VTKm_ENABLE_EXAMPLES" ON)

# VTK-m is setup by default not to export symbols unless explicitly stated.
# We prefer to only export symbols of a small set of user facing classes,
# rather than exporting all symbols. This flag is added so that consumers
# which require static builds can force all symbols on, which is something
# VTK does.
vtkm_option(VTKm_HIDE_PRIVATE_SYMBOLS "Hide symbols from libraries." ON)

vtkm_option(BUILD_SHARED_LIBS "Build VTK-m with shared libraries" ON)
set(VTKm_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

# This flag can be used to prevent VTK-m from exporting its warning flags in its
# build interface. This is useful when building VTK-m as a thirdparty library
# and the warnings are too strict for the parent project.
vtkm_option(VTKm_ENABLE_DEVELOPER_FLAGS "Enable compiler flags that are useful while developing VTK-m" ON)

# By default VTK-m would install its README.md and LICENSE.md.
# Some application might need not to install those, hence this option.
vtkm_option(VTKm_NO_INSTALL_README_LICENSE "disable the installation of README and LICENSE files" OFF)

# We are in the process of deprecating the use of virtual methods because they
# are not well supported on many accelerators. Turn this option on to remove
# the code entirely. In VTK-m 2.0 virtual methods should be removed entirely
# and this option will be removed.
vtkm_option(VTKm_NO_DEPRECATED_VIRTUAL "Do not compile support of deprecated virtual methods" ON)

# In Python wheels, having SONAME suffixes just ends up duplicating files.
# Allow VTK to turn off these symlinks for its wheel distribution.
vtkm_option(VTKm_SKIP_LIBRARY_VERSIONS "Skip versioning VTK-m libraries" OFF)

mark_as_advanced(
  VTKm_ENABLE_LOGGING
  VTKm_NO_ASSERT
  VTKm_NO_ASSERT_CUDA
  VTKm_NO_ASSERT_HIP
  VTKm_INSTALL_ONLY_LIBRARIES
  VTKm_HIDE_PRIVATE_SYMBOLS
  VTKm_ENABLE_DEVELOPER_FLAGS
  VTKm_NO_INSTALL_README_LICENSE
  VTKm_NO_DEPRECATED_VIRTUAL
  VTKm_SKIP_LIBRARY_VERSIONS
  )

#-----------------------------------------------------------------------------
# Force building of modules where specified by user-facing options.
include(VTKmModules)
vtkm_module_force_group(Core VALUE "YES" REASON "Core modules always built")
vtkm_module_force_group(Rendering ENABLE_OPTION VTKm_ENABLE_RENDERING)
vtkm_module_force_group(Logging ENABLE_OPTION VTKm_ENABLE_LOGGING)
vtkm_module_force_group(Testing
  ENABLE_OPTION VTKm_ENABLE_TESTING_LIBRARY
  ENABLE_VALUE "WANT"
  DISABLE_VALUE "DONT_WANT"
  )
vtkm_module_force_group(Benchmarking ENABLE_OPTION VTKm_ENABLE_BENCHMARKS)

# The tutorial requires several common filters. This logic might need to
# become more complicated (or less compliated if we decide to always
# compile these).
if(VTKm_ENABLE_TUTORIALS)
  vtkm_module_force_group(FiltersCommon VALUE "YES" REASON "Tutorial needs common filters.")
endif()

#-----------------------------------------------------------------------------

# Setup default build types
include(VTKmBuildType)

# Include the vtk-m wrappers
include(VTKmWrappers)

# Create vtkm_compiler_flags library. This is an interface library that
# holds all the C++ compiler flags that are needed for consumers and
# when building VTK-m.
include(VTKmCompilerFlags)


#-----------------------------------------------------------------------------


# be available for use
if (VTKm_ENABLE_TESTING)
  file(STRINGS "${VTKm_SOURCE_DIR}/README.md" sentinel_data LIMIT_COUNT 1)
  if (NOT sentinel_data STREQUAL "-- DO NOT MODIFY THIS LINE --")
    message(WARNING


      "to obtain the testing data.")
    set(VTKm_ENABLE_TESTING off)
  endif()
endif()

# We include the wrappers unconditionally as VTK-m expects the function to
# always exist (and early terminate when testing is disabled).
include(testing/VTKmTestWrappers)
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


  configure_file(CTestCustom.cmake.in
    ${VTKm_BINARY_DIR}/CTestCustom.cmake @ONLY)

  #-----------------------------------------------------------------------------
  # Find the Python interpreter, which we will use during the build process
  find_package(Python QUIET COMPONENTS Interpreter)

  #-----------------------------------------------------------------------------
  # Find Pyexpander in case somebody wants to update the auto generated
  # faux variadic template code
  find_package(Pyexpander QUIET)

  # Setup compiler flags for dynamic analysis if needed
  include(testing/VTKmCompilerDynamicAnalysisFlags)

endif()

#-----------------------------------------------------------------------------
# Check basic type sizes.
include(CheckTypeSize)

check_type_size(long VTKm_SIZE_LONG BUILTIN_TYPES_ONLY)
check_type_size("long long" VTKm_SIZE_LONG_LONG BUILTIN_TYPES_ONLY)

#-----------------------------------------------------------------------------
# Add subdirectories
add_subdirectory(vtkmstd)

#-----------------------------------------------------------------------------
# Process modules
vtkm_modules_scan(
  SCAN_DIRECTORIES vtkm benchmarking
  PROVIDED_MODULES all_modules
  )
vtkm_modules_build(
  PROVIDED_MODULES ${all_modules}
  WANT_BY_DEFAULT ${VTKm_BUILD_ALL_LIBRARIES}
  WANT_BY_DEFAULT_REASON "VTKm_BUILD_ALL_LIBRARIES is `${VTKm_BUILD_ALL_LIBRARIES}`"
  )

#-----------------------------------------------------------------------------
# Build documentation
if (VTKm_ENABLE_DOCUMENTATION)
  include(VTKmBuildDocumentation)
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

configure_file(${VTKm_SOURCE_DIR}/config/vtkm_config.mk.in
  ${VTKm_BINARY_DIR}/config/vtkm_config.mk @ONLY)
install(FILES ${VTKm_BINARY_DIR}/config/vtkm_config.mk
  DESTINATION ${VTKm_INSTALL_SHARE_DIR}
  )
configure_file(${VTKm_SOURCE_DIR}/config/vtkm.pc.in
  ${VTKm_BINARY_DIR}/config/vtkm.pc @ONLY)
install(FILES ${VTKm_BINARY_DIR}/config/vtkm.pc
  DESTINATION ${VTKm_INSTALL_SHARE_DIR}
  )

include(VTKmInstallCMakePackage)

# Install the readme and license files.
if (NOT VTKm_NO_INSTALL_README_LICENSE)
install(FILES ${VTKm_SOURCE_DIR}/README.md
  DESTINATION ${VTKm_INSTALL_SHARE_DIR}
  RENAME VTKmREADME.md
  )
install(FILES ${VTKm_SOURCE_DIR}/LICENSE.txt
  DESTINATION ${VTKm_INSTALL_SHARE_DIR}
  RENAME VTKmLICENSE.txt
  )
endif()

if(NOT VTKm_INSTALL_ONLY_LIBRARIES)
  install(
    FILES
      ${VTKm_BUILD_CMAKE_BASE_DIR}/${VTKm_INSTALL_CONFIG_DIR}/VTKmConfig.cmake
      ${VTKm_BUILD_CMAKE_BASE_DIR}/${VTKm_INSTALL_CONFIG_DIR}/VTKmConfigVersion.cmake
    DESTINATION ${VTKm_INSTALL_CONFIG_DIR}
    )

  # Install helper configure files.
  install(
    FILES
      ${VTKm_SOURCE_DIR}/CMake/VTKmCMakeBackports.cmake
      ${VTKm_SOURCE_DIR}/CMake/FindTBB.cmake
      ${VTKm_SOURCE_DIR}/CMake/patches/FindMPI.cmake
    DESTINATION ${VTKm_INSTALL_CMAKE_MODULE_DIR}
    )
  install(
    FILES
      ${VTKm_SOURCE_DIR}/CMake/patches/3.15/FindMPI.cmake
    DESTINATION ${VTKm_INSTALL_CMAKE_MODULE_DIR}/3.15
    )

  # Install support files.
  install(
    FILES
      ${VTKm_SOURCE_DIR}/CMake/VTKmCPUVectorization.cmake
      ${VTKm_SOURCE_DIR}/CMake/VTKmDetectCUDAVersion.cu
      ${VTKm_SOURCE_DIR}/CMake/VTKmDeviceAdapters.cmake
      ${VTKm_SOURCE_DIR}/CMake/VTKmDIYUtils.cmake
      ${VTKm_SOURCE_DIR}/CMake/VTKmExportHeaderTemplate.h.in
      ${VTKm_SOURCE_DIR}/CMake/VTKmRenderingContexts.cmake
      ${VTKm_SOURCE_DIR}/CMake/VTKmWrappers.cmake
    DESTINATION ${VTKm_INSTALL_CMAKE_MODULE_DIR}
    )

  # Create and install exports for external projects
  export(EXPORT ${VTKm_EXPORT_NAME}
    FILE ${VTKm_BUILD_CMAKE_BASE_DIR}/${VTKm_INSTALL_CONFIG_DIR}/VTKmTargets.cmake
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
if (VTKm_ENABLE_TESTING)

  #-----------------------------------------------------------------------------
  # Add "meta" tests that check the state of the repository
  # SystemInformation prints out information about the current configuration
  # CopyrightStatement checks that the copyright statement is in all source files
  # SourceInBuild checks that all source files are listed in the build
  # SourceInInstall checks that all source files are installed in the build
  add_test(NAME SystemInformation
    COMMAND ${CMAKE_COMMAND} "-DVTKm_SOURCE_DIR=${VTKm_SOURCE_DIR}" "-DVTKm_BINARY_DIR=${VTKm_BINARY_DIR}" -P "${VTKm_SOURCE_DIR}/CMake/testing/VTKmSystemInformation.cmake"
    )
  add_test(NAME CopyrightStatement
    COMMAND ${CMAKE_COMMAND} "-DVTKm_SOURCE_DIR=${VTKm_SOURCE_DIR}" -P "${VTKm_SOURCE_DIR}/CMake/VTKmCheckCopyright.cmake"
    )
  # increase timeout since on some machines CopyrightStatement test takes a long time.
  set_tests_properties(CopyrightStatement PROPERTIES TIMEOUT 300)

  # Setup the infrastructure to allow VTK-m to run tests against a temporary
  # installed version of VTK-m.
  include(testing/VTKmTestInstall)
  vtkm_test_install()
else ()
  set(CTEST_USE_LAUNCHERS off)
endif()

#-----------------------------------------------------------------------------
# Build examples
add_subdirectory(examples)

if (VTKm_INSTALL_EXAMPLES)
  include(GNUInstallDirs)
  install(DIRECTORY examples DESTINATION ${CMAKE_INSTALL_DOCDIR} REGEX examples/CMakeLists.txt EXCLUDE)
endif()

#-----------------------------------------------------------------------------
# Tutorial examples
if(VTKm_ENABLE_TUTORIALS)
  add_subdirectory(tutorial)
endif()
