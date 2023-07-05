FROM ubuntu:20.04
LABEL maintainer "Sujin Philip<sujin.philip@kitware.com>"

# Base dependencies for building VTK-m projects
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      cmake \
      curl \
      g++ \
      git \


      libmpich-dev \
      libomp-dev \
      mpich \
      ninja-build \
      rsync \
      ssh \
      software-properties-common



# system packaged version



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
