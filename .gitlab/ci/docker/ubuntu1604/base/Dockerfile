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

FROM ubuntu:16.04
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@gmail.com>"

# Base dependencies for building VTK-m projects
RUN apt-get update && apt-get install -y --no-install-recommends \
      autoconf \
      automake \
      autotools-dev \
      clang-3.8 \
      clang-5.0 \
      curl \
      g++ \
      g++-4.8 \
      libtbb-dev \
      make \
      ninja-build \
      pkg-config \
      software-properties-common \
      ssh

# extra dependencies for dejagore machine
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | bash
RUN apt-get install -y --no-install-recommends \
      git \
      git-lfs \
      && \
    rm -rf /var/lib/apt/lists/*

# Provide a modern OpenMPI verion that supports
# running as root via environment variables
RUN mkdir /opt/openmpi && \
    curl -L https://download.open-mpi.org/release/open-mpi/v4.0/openmpi-4.0.3.tar.gz > openmpi-4.0.3.tar.gz && \
    tar -xf openmpi-4.0.3.tar.gz && \
    cd openmpi-4.0.3 && \
    ./configure --prefix=/opt/openmpi && \
    make -j all && \
    make install

# Provide a consistent CMake path across all images
# Install CMake 3.12 as it is the minium for non-cuda builds
RUN mkdir /opt/cmake && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.12.4/cmake-3.12.4-Linux-x86_64.sh > cmake-3.12.4-Linux-x86_64.sh && \
    sh cmake-3.12.4-Linux-x86_64.sh --prefix=/opt/cmake/ --exclude-subdir --skip-license && \
    rm cmake-3.12.4-Linux-x86_64.sh

# Provide CMake 3.17 so we can re-run tests easily
# This will be used when we run just the tests
RUN mkdir /opt/cmake-latest/ && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.17.3/cmake-3.17.3-Linux-x86_64.sh > cmake-3.17.3-Linux-x86_64.sh && \
    sh cmake-3.17.3-Linux-x86_64.sh --prefix=/opt/cmake-latest/ --exclude-subdir --skip-license && \
    rm cmake-3.17.3-Linux-x86_64.sh && \
    ln -s /opt/cmake-latest/bin/ctest /opt/cmake-latest/bin/ctest-latest

ENV PATH "/opt/cmake/bin:/opt/cmake-latest/bin:${PATH}"
ENV LD_LIBRARY_PATH "/opt/openmpi/lib:${LD_LIBRARY_PATH}"
