#!/usr/bin/env bash
# shellcheck disable=SC2079

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

set -ex

version="${1:-3.23.4}"

case "$( uname -s )" in
    Linux)
        readonly -A sumsByVersion=(
            # We require CMake >= 3.13 in the CI to support CUDA builds
            ['3.13.5']='e2fd0080a6f0fc1ec84647acdcd8e0b4019770f48d83509e6a5b0b6ea27e5864'
            ['3.23.4']='3fbcbff85043d63a8a83c8bdf8bd5b1b2fd5768f922de7dc4443de7805a2670d'
        )
        shatool="sha256sum"
        sha256sum="${sumsByVersion[$version]}"
        platform="linux"
        arch="x86_64"
        ;;
    Darwin)
        shatool="shasum -a 256"
        sha256sum="98cac043cdf321caa4fd07f27da3316db6c8bc48c39997bf78e27e5c46c4eb68"
        platform="macos"
        arch="universal"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )"
        exit 1
        ;;
esac
readonly shatool
readonly sha256sum
readonly platform
readonly arch

cd .gitlab || exit

readonly tarball="cmake-$version-$platform-$arch.tar.gz"
curl -SOL "https://github.com/Kitware/CMake/releases/download/v$version/$tarball"

echo "$sha256sum  $tarball" > cmake.sha256sum
$shatool --check cmake.sha256sum

# Extract cmake install root into director named cmake
mkdir cmake
tar xf "$tarball" --strip-components=1 -C cmake

if [ "$( uname -s )" = "Darwin" ]; then
    ln -s CMake.app/Contents/bin cmake/bin
fi
