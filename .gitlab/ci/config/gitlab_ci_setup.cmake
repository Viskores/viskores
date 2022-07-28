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

if (NOT DEFINED "ENV{GITLAB_CI}")
  message(FATAL_ERROR
    "This script assumes it is being run inside of GitLab-CI")
endif ()

# Set up the source and build paths.
set(CTEST_SOURCE_DIRECTORY "$ENV{CI_PROJECT_DIR}")
set(CTEST_BINARY_DIRECTORY "${CTEST_SOURCE_DIRECTORY}/build")

if ("$ENV{VTKM_SETTINGS}" STREQUAL "")
  message(FATAL_ERROR
    "The VTKM_SETTINGS environment variable is required to know what "
    "build options should be used.")
endif ()

# Default to Release builds.
if (NOT "$ENV{CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CTEST_BUILD_CONFIGURATION "$ENV{CMAKE_BUILD_TYPE}")
endif ()
if (NOT CTEST_BUILD_CONFIGURATION)
  set(CTEST_BUILD_CONFIGURATION "Release")
endif ()

# Set the build metadata.
string(TOLOWER ${CTEST_BUILD_CONFIGURATION} build_type)
set(CTEST_BUILD_NAME "${build_type}+$ENV{VTKM_SETTINGS}")

set(site_name "$ENV{CI_JOB_NAME}")
string(REPLACE "docs:" "docs_" site_name "${site_name}")
string(REPLACE "build" "" site_name "${site_name}")
string(REPLACE "test" "" site_name "${site_name}")
string(REPLACE ":" "" site_name "${site_name}")
set(CTEST_SITE ${site_name})

# Default to using Ninja.
if (NOT "$ENV{CMAKE_GENERATOR}" STREQUAL "")
  set(CTEST_CMAKE_GENERATOR "$ENV{CMAKE_GENERATOR}")
endif ()
if (NOT CTEST_CMAKE_GENERATOR)
  set(CTEST_CMAKE_GENERATOR "Ninja")
endif ()

# Determine the track to submit to.
set(CTEST_TRACK "merge-requests")
if("$ENV{CI_COMMIT_REF_NAME}" STREQUAL "master")
  set(CTEST_TRACK "master")
elseif("$ENV{CI_COMMIT_REF_NAME}" STREQUAL "release")
  set(CTEST_TRACK "release")
endif()

if("$ENV{VTKM_CI_NIGHTLY}" STREQUAL "TRUE")
  set(CTEST_TRACK "Nightly")
endif()

# In Make, default parallelism to number of cores.
if(CTEST_CMAKE_GENERATOR STREQUAL "Unix Makefiles")
  include(ProcessorCount)
  ProcessorCount(nproc)

  if(DEFINED ENV{CTEST_MAX_PARALLELISM})
    if(nproc GREATER $ENV{CTEST_MAX_PARALLELISM})
      set(nproc $ENV{CTEST_MAX_PARALLELISM})
    endif()
  endif()

  set(CTEST_BUILD_FLAGS "-j${nproc}")
endif()

# In Ninja, we do not need to specify parallelism unless we need to restrict
# the number of threads.
if(CTEST_CMAKE_GENERATOR STREQUAL "Ninja" AND DEFINED ENV{CTEST_MAX_PARALLELISM})
  set(CTEST_BUILD_FLAGS "-j$ENV{CTEST_MAX_PARALLELISM}")
endif()

if(DEFINED ENV{CTEST_MEMORYCHECK_TYPE})
  set(env_value "$ENV{CTEST_MEMORYCHECK_TYPE}")
  list(APPEND optional_variables "set(CTEST_MEMORYCHECK_TYPE ${env_value})")
endif()

if(DEFINED ENV{CTEST_MEMORYCHECK_SANITIZER_OPTIONS})
  set(env_value "$ENV{CTEST_MEMORYCHECK_SANITIZER_OPTIONS}")
  list(APPEND optional_variables "set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS ${env_value})")
endif()

#We need to do write this information out to a file in the build directory
file(TO_CMAKE_PATH "${CTEST_SOURCE_DIRECTORY}" src_path) #converted so we can run on windows
file(TO_CMAKE_PATH "${CTEST_BINARY_DIRECTORY}" bin_path) #converted so we can run on windows

set(state
"
  set(CTEST_SOURCE_DIRECTORY \"${src_path}\")
  set(CTEST_BINARY_DIRECTORY \"${bin_path}\")

  set(CTEST_BUILD_NAME ${CTEST_BUILD_NAME})
  set(CTEST_SITE ${CTEST_SITE})

  set(CTEST_CMAKE_GENERATOR \"${CTEST_CMAKE_GENERATOR}\")
  set(CTEST_BUILD_CONFIGURATION ${CTEST_BUILD_CONFIGURATION})
  set(CTEST_BUILD_FLAGS \"${CTEST_BUILD_FLAGS}\")

  set(CTEST_TRACK ${CTEST_TRACK})

  ${optional_variables}
"
)
file(WRITE ${CTEST_BINARY_DIRECTORY}/CIState.cmake "${state}")
