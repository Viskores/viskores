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

FROM rocm/dev-ubuntu-22.04
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@gmail.com>"

# Base dependencies for building VTK-m projects
RUN apt update && \
    DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
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
    rocthrust-dev \
    && \
    apt clean

# Need to run git-lfs install manually on ubuntu based images when using the
# system packaged version
RUN git-lfs install

# Provide CCACHE
ENV CCACHE_DIR "/ccache"
ENV PATH "/opt/ccache/bin:${PATH}"
ARG CCACHE_VERSION=4.6.1
RUN mkdir /opt/ccache/ && \
    curl -L https://github.com/ccache/ccache/releases/download/v$CCACHE_VERSION/ccache-$CCACHE_VERSION-linux-x86_64.tar.xz | tar -vxJ && \
    make -C ccache-$CCACHE_VERSION-linux-x86_64 install prefix=/opt/ccache && \
    rm -rf ccache-$CCACHE_VERSION-linux-x86_64 && \
    ccache -z && ccache -s

# Provide CMake
ARG CMAKE_VERSION=3.21.1
RUN mkdir /opt/cmake/ && \
    curl -L https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION-Linux-x86_64.sh > cmake-$CMAKE_VERSION-Linux-x86_64.sh && \
    sh cmake-$CMAKE_VERSION-Linux-x86_64.sh --prefix=/opt/cmake/ --exclude-subdir --skip-license && \
    rm cmake-$CMAKE_VERSION-Linux-x86_64.sh && \
    ln -s /opt/cmake/bin/ctest /opt/cmake/bin/ctest-latest

ENV PATH "/opt/cmake/bin:${PATH}"
ENV CMAKE_PREFIX_PATH "/opt/rocm/lib/cmake:/opt/rocm/lib:${CMAKE_PREFIX_PATH}"
ENV CMAKE_GENERATOR "Ninja"

ENV KOKKOS_VERSION=3.7.01
COPY kokkos_cmake_config.cmake kokkos_cmake_config.cmake
RUN curl -L https://github.com/kokkos/kokkos/archive/refs/tags/$KOKKOS_VERSION.tar.gz | tar -xzf - && \
    cmake -S kokkos-$KOKKOS_VERSION -B build -C kokkos_cmake_config.cmake                             \
       -DCMAKE_PREFIX_INSTALL=/opt/kokkos/$KOKKOS_VERSION                                             \
       -DKokkos_ARCH_VEGA900=ON                                                                    && \
    cmake --build build -v                                                                         && \
    cmake --install build                                                                          && \
    rm -rf build kokkos-$KOKKOS_VERSION

ENV KOKKOS_VERSION=4.3.01
COPY kokkos_cmake_config.cmake kokkos_cmake_config.cmake
RUN curl -L https://github.com/kokkos/kokkos/archive/refs/tags/$KOKKOS_VERSION.tar.gz | tar -xzf - && \
    cmake -S kokkos-$KOKKOS_VERSION -B build -C kokkos_cmake_config.cmake                             \
       -DCMAKE_PREFIX_INSTALL=/opt/kokkos/$KOKKOS_VERSION                                             \
       -DKokkos_ARCH_VEGA906=ON                                                                    && \
    cmake --build build -v                                                                         && \
    cmake --install build                                                                          && \
    rm -rf build kokkos-$KOKKOS_VERSION
