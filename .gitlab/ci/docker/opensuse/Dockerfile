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


      hdf5-devel \
      libgomp1 \
      libucp-devel \
      mpich-ofi \
      mpich-ofi-devel \
      ninja \
      openucx-tools \
      python311 \
      python311-scipy \
      tbb-devel && \
    zypper clean --all

ENV fI_PROVIDER=sockets





