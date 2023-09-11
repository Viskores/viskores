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

FROM nvidia/cuda:11.7.1-devel-ubuntu18.04
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@gmail.com>"

# Base dependencies for building VTK-m projects
RUN apt-get update && apt-get install -y --no-install-recommends \
      clang-8 \
      curl \
      g++-8 \
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

# Install Git LFS from official tarball, repo version is too old








