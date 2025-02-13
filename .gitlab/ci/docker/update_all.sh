#!/bin/sh

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
readonly date="$(date +%Y%m%d)"

podman build -t ghcr.io/viskores/viskores:ci-ubuntu2004-$date -f ubuntu2004.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-doxygen-$date -f ubuntu2004_doxygen.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-ubuntu2004-cuda18.8-$date -f ubuntu2004_cuda11.8.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-ubuntu2004-kokkos-$date -f ubuntu2004_kokkos.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-ubuntu2204-$date -f ubuntu2204.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-ubuntu2204-cuda12.2-$date -f ubuntu2204_cuda12.2.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-ubuntu2204-kokkos-cuda-$date -f ubuntu2204_kokkos_cuda.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-ubuntu2204-kokkos-hip-$date -f ubuntu2204_kokkos_hip.dockerfile .

podman push ghcr.io/viskores/viskores:ci-ubuntu2004-$date
podman push ghcr.io/viskores/viskores:ci-doxygen-$date
podman push ghcr.io/viskores/viskores:ci-ubuntu2004-cuda18.8-$date
podman push ghcr.io/viskores/viskores:ci-ubuntu2004-kokkos-$date
podman push ghcr.io/viskores/viskores:ci-ubuntu2204-$date
podman push ghcr.io/viskores/viskores:ci-ubuntu2204-cuda12.2-$date
podman push ghcr.io/viskores/viskores:ci-ubuntu2204-kokkos-cuda-$date
podman push ghcr.io/viskores/viskores:ci-ubuntu2204-kokkos-hip-$date
