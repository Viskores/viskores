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

FROM docker.io/ubuntu:22.04
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@gmail.com>"

RUN apt-get update && apt-get install -y --no-install-recommends \
      cmake \
      curl \
      g++ \
      g++-10 \
      g++-12 \
      clang-11 \
      clang-12 \
      git \
      libmpich-dev \
      libomp-dev \
      libtbb-dev \
      libhdf5-dev \
      make \
      mpich \
      ninja-build \
      pkg-config \
      software-properties-common
