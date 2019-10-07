#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="lcl"
readonly ownership="Lightweight Cell Library Upstream <kwrobot@kitware.com>"
readonly subtree="vtkm/thirdparty/$name/vtkm$name"
readonly repo="https://gitlab.kitware.com/vtk/lcl.git"
readonly tag="master"
readonly paths="
lcl
LICENSE.md
README.md
"

extract_source () {
    git_archive
    pushd "${extractdir}/${name}-reduced"
    rm -rf lcl/testing
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
