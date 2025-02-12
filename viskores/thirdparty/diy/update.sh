#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="diy"
readonly ownership="Diy Upstream <kwrobot@kitware.com>"
readonly subtree="viskores/thirdparty/$name/viskores$name"
readonly repo="https://gitlab.kitware.com/third-party/diy2.git"
readonly tag="for/viskores-20250206-g9a1c294e"
readonly paths="
cmake
include
CMakeLists.txt
LEGAL.txt
LICENSE.txt
README.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv include/diy include/viskoresdiy
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
