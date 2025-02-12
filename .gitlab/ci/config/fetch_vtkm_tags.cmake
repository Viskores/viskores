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

## Fetch latests tags from Viskores main repo

find_package(Git)
if(NOT Git_FOUND)
  message(ERROR "Git not installed, Could not fetch vtk/viskores tags")
  return()
endif()

set(REPO_URL "https://gitlab.kitware.com/vtk/viskores.git")

## Only fetch tags when in a fork in a MR since often times forks do not have
## the latest tags from the main repo.
if(DEFINED ENV{CI_MERGE_REQUEST_ID} AND NOT $ENV{CI_REPOSITORY_URL} MATCHES "vtk/viskores\\.git$")
  message("Fetching vtk/viskores repo latest tags")
  execute_process(
    COMMAND
    ${GIT_EXECUTABLE}
    fetch
    ${REPO_URL}
    "refs/tags/*:refs/tags/*"
  )
endif()
