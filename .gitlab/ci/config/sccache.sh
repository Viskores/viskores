#!/bin/sh

set -e

readonly version="nvcc_v4"
readonly sha256sum="260779b4a740fe8373d251d1e318541a98dd5cd2f8051eedd55227a5a852fdf7"
readonly filename="sccache-0.2.14-$version-x86_64-unknown-linux-musl"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > sccache.sha256sum
curl -OL "https://github.com/robertmaynard/sccache/releases/download/$version/$tarball"
sha256sum --check sccache.sha256sum
tar xf "$tarball"
#mv "$filename/sccache" .
