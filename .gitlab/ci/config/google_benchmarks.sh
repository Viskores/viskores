#!/bin/bash

set -xe

readonly version="v1.5.2"
readonly tarball="$version.tar.gz"
readonly url="https://github.com/google/benchmark/archive/$tarball"
readonly sha256sum="dccbdab796baa1043f04982147e67bb6e118fe610da2c65f88912d73987e700c"
readonly install_dir="$HOME/gbench"

if ! [[ "$VTKM_SETTINGS" =~ "benchmarks" ]]; then
  exit 0
fi

cd "$HOME"

echo "$sha256sum  $tarball" > gbenchs.sha256sum
curl --insecure -OL "$url"
sha256sum --check gbenchs.sha256sum
tar xf "$tarball"

mkdir build
mkdir "$install_dir"

cmake -GNinja -S benchmark* -B build -DBENCHMARK_DOWNLOAD_DEPENDENCIES=ON
cmake --build build
cmake --install build --prefix "$install_dir"
