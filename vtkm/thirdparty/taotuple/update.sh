#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="taotuple"
readonly ownership="TaoCpp Tuple Upstream <kwrobot@kitware.com>"
readonly subtree="vtkm/thirdparty/$name/vtkm$name"
readonly repo="https://gitlab.kitware.com/third-party/$name.git"
readonly tag="for/vtk-m"
readonly paths="
include
git-subtree
LICENSE
README.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    rm include/tao/seq
    cp -r git-subtree/sequences/include/tao/seq include/tao/
    rm -rf git-subtree
    popd

}

. "${BASH_SOURCE%/*}/../update-common.sh"
