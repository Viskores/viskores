#!/bin/sh

set -e

readonly version="nvcc_v2"
readonly sha256sum="923e919ffdf3a9d7bdee79ddaea0169fc1f41dd2f2cac2c0b160b7a21d9c459d"
readonly filename="sccache-0.2.14-$version-x86_64-unknown-linux-musl"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > sccache.sha256sum
curl -OL "https://github.com/robertmaynard/sccache/releases/download/$version/$tarball"
sha256sum --check sccache.sha256sum
tar xf "$tarball"
#mv "$filename/sccache" .
