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

FROM docker.io/ubuntu:20.04
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@gmail.com>"

ENV TZ=America/New_York

# Base dependencies for building VTK-m projects
RUN apt update && DEBIAN_FRONTEND="noninteractive" apt install -y --no-install-recommends \
      clang-6.0 \
      cmake \
      curl \
      g++ \
      g++-9 \
      git \
      git-lfs \
      libmpich-dev \
      libomp-dev \
      libtbb-dev \
      libhdf5-dev \
      make \
      mpich \
      ninja-build \
      pkg-config \
      python \
      python3-scipy \
      software-properties-common && \
      apt clean

# Need to run git-lfs install manually on ubuntu based images when using the
# system packaged version
RUN git-lfs install
