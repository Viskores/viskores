#!/bin/bash -e

declare -r POLL_INTERVAL_SECONDS="10"

function fetch_commit_status()
{
  local -r url="$1"
  local output

  output=$(curl --insecure --silent "$url" | tr ',{}[]' '\n' | grep -Po -m1 '(?<=^"status":")\w+(?=")')

  # No status means that the pipeline has not being created yet
  [ -z "$output" ] && output="empty"
  echo "$output"
}

function wait_commit_pipeline_status()
{
  local -r base_url="$1"
  local -r commit="$2"
  local -r url="${base_url}/repository/commits/${commit}"

  while true
  do
    local ret="$(fetch_commit_status "$url")"
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
