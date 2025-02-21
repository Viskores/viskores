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

FROM docker.io/nvidia/cuda:12.2.2-devel-ubuntu22.04
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@gmail.com>"

# Base dependencies for building VTK-m projects
RUN apt-get update && apt-get install -y --no-install-recommends \
      clang-11 \
      curl \
      g++-10 \
      git \
      libmpich-dev \
      libomp-dev \
      libtbb-dev \
      make \
      mpich \
      ninja-build \
      pkg-config \
      python3 \
      python3-scipy \
      && \
    rm -rf /var/lib/apt/lists/*
