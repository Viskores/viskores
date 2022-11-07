#!/bin/bash -ex

git -c http.sslVerify=false push --delete "$1" "$2"
