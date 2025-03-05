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

# In the gitlab CI our PRs branches tip commit is not the head commit of the
# PR, it is instead the so called merged_commit_sha as described in the GitHub
# Rest API for pull requests. We need to report to the CDASH the original
# commit thus, we set it here using the ORIGINAL_COMMIT_SHA file.

set(ref_name $ENV{CI_COMMIT_REF_NAME})
set(commit_sha $ENV{CI_COMMIT_SHA})

if(ref_name MATCHES "^pr[0-9]+_.*$")
  execute_process(
    COMMAND git rev-parse "${commit_sha}^2"
    OUTPUT_FILE ORIGINAL_COMMIT_SHA
    RESULT_VARIABLE exit_code
  )
  if (exit_code)
    message(FATAL_ERROR "Could not find original commit EC=${exit_code}")
  endif()
endif()
