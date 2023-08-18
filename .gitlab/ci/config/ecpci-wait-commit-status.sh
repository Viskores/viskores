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

declare -r POLL_INTERVAL_SECONDS="10"

function fetch_commit_status()
{
  local -r url="$1"
  local -r commit="$2"

  local output=$(curl --insecure --silent "${url}/repository/commits/${commit}" \
    | tr ',{}[]' '\n' \
    | grep -Po -m1 '(?<=^"status":")\w+(?=")' \
  )

  # No status means that the pipeline has not being created yet
  [ -z "$output" ] && output="empty"
  echo "$output"
}

function print_pipeline_url()
{
  local -r url="$1"
  local -r commit="$2"

  local web_url=$(curl --insecure --silent "${url}" \
    | tr ',{}[]' '\n' \
    | grep -Po -m1 '(?<=^"web_url":").+(?=")' \
  )

  local pipeline_id=$(curl --insecure --silent "${url}/repository/commits/${commit}" \
     | tr ',{}[]' '\n' \
     | grep -Po '(?<=^"id":)\d+$' \
   )

  echo "######################################################################"
  echo "ECP Pipeline: ${web_url}/-/pipelines/$pipeline_id"
  echo "######################################################################"
}

function wait_commit_pipeline_status()
{
  local -r base_url="$1"
  local -r commit="$2"
  local is_url_printed="no"

  while true
  do
    local ret="$(fetch_commit_status "$base_url" "$commit")"

    if [ "$ret" != "empty" ] && [ "$is_url_printed" == "no" ]
    then
      print_pipeline_url "$base_url" "$commit"
      is_url_printed="yes"
    fi

    case "$ret" in
      success)
        return 0 ;;
      failed|canceled|skipped)
        echo "ERROR: The pipeline exited with \`${ret}\` status" > /dev/stderr
        return 1 ;;
    esac
    sleep "$POLL_INTERVAL_SECONDS"
  done
}

wait_commit_pipeline_status "$1" "$2"
