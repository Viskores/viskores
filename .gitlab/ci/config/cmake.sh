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

version="${1:-3.30.2}"

case "$( uname -s )" in
    Linux)
        readonly -A sumsByVersion=(
            # We require CMake >= 3.13 in the CI to support CUDA builds
            ['3.15.5']='03cfd669d0f990040ec89bb63a3ae7f6d61fd17c1c4d5e7ec3d1a35fe1f043f0'
            ['3.30.2']='cdd7fb352605cee3ae53b0e18b5929b642900e33d6b0173e19f6d4f2067ebf16'
        )
        shatool="sha256sum"
        sha256sum="${sumsByVersion[$version]}"
        platform="linux"
        arch="x86_64"
        ;;
    Darwin)
        shatool="shasum -a 256"
        sha256sum="c6fdda745f9ce69bca048e91955c7d043ba905d6388a62e0ff52b681ac17183c"
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
