##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

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

# Build and install ANARI SDK
WORKDIR /opt/anari/src
ARG ANARI_VERSION=0.15.0
RUN curl -L https://github.com/KhronosGroup/ANARI-SDK/archive/refs/tags/v$ANARI_VERSION.tar.gz | tar xzv && \
    cmake -GNinja \
      -S ANARI-SDK-$ANARI_VERSION \
      -B build \
      -DBUILD_CTS=OFF \
      -DBUILD_EXAMPLES=OFF \
      -DBUILD_HELIDE_DEVICE=ON \
      -DBUILD_REMOTE_DEVICE=OFF \
      -DBUILD_SHARED_LIBS=ON \
      -DBUILD_TESTING=OFF \
      -DBUILD_VIEWER=OFF \
      -DCMAKE_INSTALL_PREFIX=/opt/anari \
      -DINSTALL_VIEWER_LIBRARY=OFF && \
    cmake --build build && \
    cmake --install build && \
    rm -rf *

WORKDIR /root
