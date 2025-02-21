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

FROM docker.io/ubuntu:24.04
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@gmail.com>"

RUN apt-get update && apt-get install -y --no-install-recommends \
      cmake \
      curl \
      g++ \
      g++-13 \
      g++-14 \
      git \
      libmpich-dev \
      libomp-dev \
      libtbb-dev \
      libhdf5-dev \
      make \
      mpich \
      ninja-build \
      pkg-config \
      software-properties-common \
      && \
    rm -rf /var/lib/apt/lists/*
