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

FROM ubuntu:20.04
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@gmail.com>"

# Base dependencies for building VTK-m projects
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      cmake \
      curl \
      g++ \
      git \
      git-lfs \
      libmpich-dev \
      libomp-dev \
      make \
      mpich \
      ninja-build \
      pkg-config \
      rsync \
      ssh \
      software-properties-common

# Need to run git-lfs install manually on ubuntu based images when using the
# system packaged version
RUN git-lfs install

# Provide CMake 3.17 so we can re-run tests easily
# This will be used when we run just the tests
RUN mkdir /opt/cmake/ && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.21.2/cmake-3.21.2-Linux-x86_64.sh > cmake-3.21.2-Linux-x86_64.sh && \
    sh cmake-3.21.2-Linux-x86_64.sh --prefix=/opt/cmake/ --exclude-subdir --skip-license && \
    rm cmake-3.21.2-Linux-x86_64.sh && \
    ln -s /opt/cmake/bin/ctest /opt/cmake/bin/ctest-latest

ENV PATH "${PATH}:/opt/cmake/bin"

# Build and install Kokkos
ARG KOKKOS_VERSION=4.1.00
RUN mkdir -p /opt/kokkos/build && \
    cd /opt/kokkos/build && \
    curl -L https://github.com/kokkos/kokkos/archive/refs/tags/$KOKKOS_VERSION.tar.gz > kokkos-$KOKKOS_VERSION.tar.gz && \
    tar -xf kokkos-$KOKKOS_VERSION.tar.gz && \
    mkdir bld && cd bld && \
    cmake -GNinja -DCMAKE_INSTALL_PREFIX=/opt/kokkos -DCMAKE_CXX_FLAGS=-fPIC -DKokkos_ENABLE_SERIAL=ON ../kokkos-$KOKKOS_VERSION &&\
    ninja all && \
    ninja install

# Build and install ANARI SDK
WORKDIR /opt/anari/src
ARG ANARI_VERSION=0.8.0
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
