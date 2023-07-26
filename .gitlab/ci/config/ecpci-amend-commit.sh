#!/bin/bash -e
# shellcheck disable=SC2155

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

git rev-parse @ > ./ORIGINAL_COMMIT_SHA
git add ./ORIGINAL_COMMIT_SHA

readonly name="$(git show --quiet --format='%cn')"
readonly email="$(git show --quiet --format='%ce')"

git config --global user.name  "$name"
git config --global user.email "$email"
git commit --amend --no-edit -a

git rev-parse @ > "$1"
