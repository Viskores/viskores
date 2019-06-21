#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="lodepng"
readonly ownership="LodePNG Upstream <kwrobot@kitware.com>"
readonly subtree="vtkm/thirdparty/$name/vtkm$name"
readonly repo="https://gitlab.kitware.com/third-party/$name.git"
readonly tag="for/vtk-m"
readonly paths="
LICENSE
lodepng.cpp
lodepng.h
README.md
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
