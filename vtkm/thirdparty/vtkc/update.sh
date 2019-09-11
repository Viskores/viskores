#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="vtkc"
readonly ownership="VTK-c Upstream <kwrobot@kitware.com>"
readonly subtree="vtkm/thirdparty/$name/vtkm$name"
readonly repo="https://gitlab.kitware.com/sujin.philip/vtk-c.git"
readonly tag="master"
readonly paths="
vtkc
LICENSE.md
README.md
"

extract_source () {
    git_archive
    pushd "${extractdir}/${name}-reduced"
    rm -rf vtkc/testing
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
