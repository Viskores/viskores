#!/bin/bash

##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

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

set -xe

readonly version="v1.6.1"
readonly tarball="$version.tar.gz"
readonly url="https://github.com/google/benchmark/archive/$tarball"
readonly sha256sum="6132883bc8c9b0df5375b16ab520fac1a85dc9e4cf5be59480448ece74b278d4"
readonly install_dir="$HOME/gbench"

if ! [[ "$VISKORES_SETTINGS" =~ "benchmarks" ]]; then
  exit 0
fi

cd "$HOME"

echo "$sha256sum  $tarball" > gbenchs.sha256sum
curl --insecure -OL "$url"
sha256sum --check gbenchs.sha256sum
tar xf "$tarball"

mkdir build
mkdir "$install_dir"

cmake -GNinja -S benchmark* -B build   \
  -DBENCHMARK_DOWNLOAD_DEPENDENCIES=ON \
  -DCMAKE_BUILD_TYPE="Release"

cmake --build build
cmake --install build --prefix "$install_dir"
