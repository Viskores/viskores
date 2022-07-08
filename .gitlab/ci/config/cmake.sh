#!/usr/bin/env bash
# shellcheck disable=SC2079

set -x

version="${1:-3.21.1}"

case "$( uname -s )" in
    Linux)
        shatool="sha256sum"
        # We require CMake >= 3.13 in the CI to support CUDA builds
        readonly -A linuxParamsByVersion=(
        ['3.13.5']='e2fd0080a6f0fc1ec84647acdcd8e0b4019770f48d83509e6a5b0b6ea27e5864	Linux'
        ['3.21.1']='bf496ce869d0aa8c1f57e4d1a2e50c8f2fb12a6cd7ccb37ad743bb88f6b76a1e	linux'
        )

        if [ -z "${linuxParamsByVersion[$version]}" ]
        then
          echo "Given version ($version) is unsupported"
          exit 1
        fi
        sha256sum=$(cut -f 1 <<<"${linuxParamsByVersion[$version]}")
        platform=$(cut -f 2 <<<"${linuxParamsByVersion[$version]}")
        arch="x86_64"
        ;;
    Darwin)
        shatool="shasum -a 256"
        sha256sum="9dc2978c4d94a44f71336fa88c15bb0eee47cf44b6ece51b10d1dfae95f82279"
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

readonly filename="cmake-$version-$platform-$arch"
readonly tarball="$filename.tar.gz"

cd .gitlab || exit

echo "$sha256sum  $tarball" > cmake.sha256sum
curl -OL "https://github.com/Kitware/CMake/releases/download/v$version/$tarball"
$shatool --check cmake.sha256sum
tar xf "$tarball"
mv "$filename" cmake

if [ "$( uname -s )" = "Darwin" ]; then
    ln -s CMake.app/Contents/bin cmake/bin
fi
