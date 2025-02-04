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

FROM nvidia/cuda:11.6.2-devel-ubuntu18.04
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
      && \
    rm -rf /var/lib/apt/lists/*

# Need to run git-lfs install manually on ubuntu based images when using the
# system packaged version
RUN git-lfs install

# kokkos backend requires cmake 3.18
RUN mkdir /opt/cmake/ && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.21.2/cmake-3.21.2-Linux-x86_64.sh > cmake-3.21.2-Linux-x86_64.sh && \
    sh cmake-3.21.2-Linux-x86_64.sh --prefix=/opt/cmake/ --exclude-subdir --skip-license && \
    rm cmake-3.21.2-Linux-x86_64.sh && \
    ln -s /opt/cmake/bin/ctest /opt/cmake/bin/ctest-latest

ENV PATH "/opt/cmake/bin:${PATH}"

# Build and install Kokkos
ARG KOKKOS_VERSION=3.7.01
RUN mkdir -p /opt/kokkos/build && \
    cd /opt/kokkos/build && \
    curl -L https://github.com/kokkos/kokkos/archive/refs/tags/$KOKKOS_VERSION.tar.gz > kokkos-$KOKKOS_VERSION.tar.gz && \
    tar -xf kokkos-$KOKKOS_VERSION.tar.gz && \
    mkdir bld && cd bld && \
    CXX=/opt/kokkos/build/kokkos-$KOKKOS_VERSION/bin/nvcc_wrapper \
    cmake -B . -S ../kokkos-$KOKKOS_VERSION \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/opt/kokkos \
          -DCMAKE_CXX_FLAGS=-fPIC \
          -DCMAKE_CXX_STANDARD=14 \
          -DKokkos_ENABLE_CUDA=ON \
          -DKokkos_ENABLE_CUDA_CONSTEXPR=ON \
          -DKokkos_ENABLE_CUDA_LAMBDA=ON \
          -DKokkos_ENABLE_CUDA_LDG_INTRINSIC=ON \
          -DKokkos_ENABLE_CUDA_RELOCATABLE_DEVICE_CODE=OFF \
          -DKokkos_ENABLE_CUDA_UVM=ON \
          -DKokkos_ARCH_TURING75=ON && \
    cmake --build . -j 8 && \
    cmake --install .
