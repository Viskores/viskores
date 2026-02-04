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

FROM docker.io/nvidia/cuda:12.2.2-devel-ubuntu22.04
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@gmail.com>"

# Base dependencies for building VTK-m projects
RUN apt-get update && apt-get install -y --no-install-recommends \
      curl \
      g++ \
      git \
      git-lfs \
      make \
      ninja-build \
      pkg-config \
      python3 \
      python3-scipy \
      && \
    rm -rf /var/lib/apt/lists/*

# kokkos backend requires cmake 3.18
RUN mkdir /opt/cmake/ && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.21.2/cmake-3.21.2-Linux-x86_64.sh > cmake-3.21.2-Linux-x86_64.sh && \
    sh cmake-3.21.2-Linux-x86_64.sh --prefix=/opt/cmake/ --exclude-subdir --skip-license && \
    rm cmake-3.21.2-Linux-x86_64.sh && \
    ln -s /opt/cmake/bin/ctest /opt/cmake/bin/ctest-latest

ENV PATH "/opt/cmake/bin:${PATH}"

# Build and install Kokkos
ARG KOKKOS_VERSION=4.1.00
RUN mkdir -p /opt/kokkos/build && \
    cd /opt/kokkos/build && \
    curl -L https://github.com/kokkos/kokkos/archive/refs/tags/$KOKKOS_VERSION.tar.gz > kokkos-$KOKKOS_VERSION.tar.gz && \
    tar -xf kokkos-$KOKKOS_VERSION.tar.gz && \
    mkdir bld && cd bld && \
    CXX=/opt/kokkos/build/kokkos-$KOKKOS_VERSION/bin/nvcc_wrapper \
    cmake -B . -S ../kokkos-$KOKKOS_VERSION \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/opt/kokkos \
          -DKokkos_ENABLE_CUDA=ON \
          -DKokkos_ENABLE_CUDA_CONSTEXPR=ON \
          -DKokkos_ENABLE_CUDA_LAMBDA=ON \
          -DKokkos_ENABLE_CUDA_RELOCATABLE_DEVICE_CODE=OFF \
          -DKokkos_ARCH_AMPERE80=ON && \
    cmake --build . -j 8 && \
    cmake --install . && \
    cd ..; rm -rf kokkos-$KOKKOS_VERSION.tar.gz kokkos-$KOKKOS_VERSION bld
