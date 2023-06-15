#!/bin/bash -ex

git -c http.sslVerify=false push --no-verify -f "$1" "HEAD:refs/heads/${2}"
