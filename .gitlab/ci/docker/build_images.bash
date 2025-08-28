##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

#!/bin/bash

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

# data is expected to be a string of the form YYYYMMDD
readonly DATESTAMP="$(date +%Y%m%d)"
images=""

function build_image() {
  local path="${1}"
  local name="$(basename -s .dockerfile ${path})"
  local tag="ghcr.io/viskores/viskores:ci-${name}-${DATESTAMP}"
  images+="${tag}\n"
  echo "Building new image ${tag}"
  podman build -t "${tag}" -f "${path}" .
	podman push "${tag}"
}

function build_all_images() {
  build_image almalinux8.dockerfile
  build_image sync.dockerfile
  build_image ubuntu2004_cuda11.8.dockerfile
  build_image ubuntu2004.dockerfile
  build_image ubuntu2004_doxygen.dockerfile
  build_image ubuntu2004_kokkos.dockerfile
  build_image ubuntu2204_cuda12.2.dockerfile
  build_image ubuntu2204.dockerfile
  build_image ubuntu2204_kokkos_cuda.dockerfile
  build_image ubuntu2204_kokkos_hip.dockerfile
  build_image ubuntu2404.dockerfile
  build_image ubuntu2404_cuda13.0.dockerfile
}

if [ "$#" -eq 0 ]; then
  exit 1
fi

arg_1="${1}"

if [ "${arg_1}" == "-a" ]; then
  build_all_images
else
  files="$(find . -iname "${arg_1}")"
  for file in ${files}; do
    build_image "${file}"
  done
fi

echo -e "Built images:\n${images}"
