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


set(env_vars
"\
GITLAB_CI=TRUE
CI_PROJECT_DIR=$ENV{GITHUB_WORKSPACE}
CI_JOB_NAME=$ENV{GITHUB_JOB}
CI_COMMIT_REF_NAME=$ENV{GITHUB_REF_NAME}
CI_COMMIT_SHA=$ENV{GITHUB_SHA}
CI_PROJECT_DIR=$ENV{GITHUB_WORKSPACE}\
"
)

if (NOT EXISTS $ENV{GITHUB_ENV})
  message(FATAL_ERROR "GITHUB_ENV not found")
endif()

file(APPEND $ENV{GITHUB_ENV} ${env_vars})
