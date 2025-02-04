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

FROM opensuse/tumbleweed
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@kitware.com>"

# Base dependencies for building VTK-m projects
RUN zypper refresh && \
    zypper update -y && \
    zypper install -y --no-recommends \
      cmake \
      curl \
      gcc13-c++ \
      git \
      git-lfs \
      hdf5-devel \
      libgomp1 \
      make \
      ninja \
      python311 \
      python311-scipy \
      tbb-devel && \
    zypper clean --all

ARG MPICH_VERSION=4.1.1
RUN curl -s -L https://www.mpich.org/static/downloads/${MPICH_VERSION}/mpich-${MPICH_VERSION}.tar.gz | tar xzf - && \
    cd mpich-${MPICH_VERSION} && \
    ./configure \
      --disable-fortran \
      --prefix=/usr/local \
      --with-device=ch3:sock:tcp && \
    make -j $(nproc) && \
    make install && \
    rm -rf mpich-${MPICH_VERSION}

# Need to run git-lfs install manually on system packaged version
RUN git-lfs install
