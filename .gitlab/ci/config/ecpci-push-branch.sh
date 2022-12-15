#!/bin/bash -ex



git -c http.sslVerify=false push -f "$1" "HEAD:refs/heads/${2}"
