#!/bin/sh

set -e

readonly version="nvcc_v3"
readonly sha256sum="d5b56dd9e7d4597f4a47a90d6327e30a259151b59b897607e1804d6d3513f491"
readonly filename="sccache-0.2.14-$version-x86_64-unknown-linux-musl"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > sccache.sha256sum
curl -OL "https://github.com/robertmaynard/sccache/releases/download/$version/$tarball"
sha256sum --check sccache.sha256sum
tar xf "$tarball"
#mv "$filename/sccache" .
