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

podman build -t ghcr.io/viskores/viskores:ci-ubuntu2004-$date-a -f ubuntu2004.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-doxygen-$date-a -f ubuntu2004_doxygen.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-ubuntu2004-cuda11.8-$date-a -f ubuntu2004_cuda11.8.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-ubuntu2004-kokkos-$date-a -f ubuntu2004_kokkos.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-ubuntu2204-$date-a -f ubuntu2204.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-ubuntu2204-cuda12.2-$date-a -f ubuntu2204_cuda12.2.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-ubuntu2204-cuda12.2-kokkos-$date-a -f ubuntu2204_kokkos_cuda.dockerfile .
podman build -t ghcr.io/viskores/viskores:ci-ubuntu2204-hip-kokkos-$date-a -f ubuntu2204_kokkos_hip.dockerfile .

podman push ghcr.io/viskores/viskores:ci-ubuntu2004-$date-a
podman push ghcr.io/viskores/viskores:ci-doxygen-$date-a
podman push ghcr.io/viskores/viskores:ci-ubuntu2004-cuda11.8-$date-a
podman push ghcr.io/viskores/viskores:ci-ubuntu2004-kokkos-$date-a
podman push ghcr.io/viskores/viskores:ci-ubuntu2204-$date-a
podman push ghcr.io/viskores/viskores:ci-ubuntu2204-cuda12.2-$date-a
podman push ghcr.io/viskores/viskores:ci-ubuntu2204-cuda12.2-kokkos-$date-a
podman push ghcr.io/viskores/viskores:ci-ubuntu2204-hip-kokkos-$date-a
