##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##=============================================================================

# Read the files from the build directory that contain
# host information ( name, parallel level, etc )
include("$ENV{CI_PROJECT_DIR}/build/CIState.cmake")
ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")

set(cmake_args
  -C "${CMAKE_CURRENT_LIST_DIR}/config/initial_config.cmake")

# Create an entry in CDash.
ctest_start(Experimental TRACK "${CTEST_TRACK}")

# Gather update information.
find_package(Git)
set(CTEST_UPDATE_VERSION_ONLY ON)
set(CTEST_UPDATE_COMMAND "${GIT_EXECUTABLE}")

# Don't do updates when running via reproduce_ci_env.py
if(NOT DEFINED ENV{GITLAB_CI_EMULATION})
  ctest_update()
endif()

# Configure the project.
ctest_configure(APPEND
  OPTIONS "${cmake_args}"
  RETURN_VALUE configure_result)

# We can now submit because we've configured.
if(NOT DEFINED ENV{GITLAB_CI_EMULATION})
  set(CTEST_NOTES_FILES "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt")
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
    ctest_submit(PARTS Update BUILD_ID build_id)
    message(STATUS "Update submission build_id: ${build_id}")
    ctest_submit(PARTS Configure Notes BUILD_ID build_id)
    message(STATUS "Configure submission build_id: ${build_id}")
  else()
    ctest_submit(PARTS Update)
    ctest_submit(PARTS Configure Notes)
  endif()
endif()

if (configure_result)
  message(FATAL_ERROR
    "Failed to configure")
endif ()
