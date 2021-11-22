#!/bin/bash

set -xe

readonly version="91ed7eea6856f8785139c58fbcc827e82579243c"
readonly tarball="$version.tar.gz"
readonly url="https://github.com/google/benchmark/archive/$tarball"
readonly sha256sum="039054b7919b0af1082b121df35f4c24fccdd97f308e3dc28f36a0d3a3c64c69"
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
