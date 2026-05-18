#!/usr/bin/env bash
##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

# Check that no installed header has a relative path longer than MAX_PATH_LEN.
#
# Windows imposes a MAX_PATH limit of 260 characters on file paths.
# When conda installs viskores, headers land under a prefix of the form:
#
#   <cache>\https\conda.anaconda.org\conda-forge\win-64\viskores-X.Y-<hash>\Library\include\viskores-X.Y\
#
# The shortest possible such prefix (using a one-character cache dir such as
# C:\p) is ~104 characters, leaving at most 156 characters for the relative
# header path inside the source tree.  We use 130 as the enforced limit to
# provide a 26-character safety margin against even shorter future prefixes or
# deeper install layouts.  See issue #318 for the original report.

set -euo pipefail

readonly MAX_PATH_LEN=130
readonly ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly HEADER_DIR="${ROOT_DIR}/viskores"

offenders=($(
  find "${HEADER_DIR}" -name "*.h" -printf '%P\n' \
    | awk -v max="${MAX_PATH_LEN}" 'length > max' \
    | sort
))

if (( ${#offenders[@]} > 0 )); then
  echo "ERROR: the following header paths exceed ${MAX_PATH_LEN} characters"
  echo "       and will fail to install on Windows without Long Path support:"
  printf '  %s\n' "${offenders[@]}"
  exit 1
fi

echo "OK: all header paths are within ${MAX_PATH_LEN} characters."
