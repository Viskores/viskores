FROM rocm/dev-ubuntu-20.04
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@kitware.com>"

# Base dependencies for building VTK-m projects
RUN apt update && \
    DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
    curl \
    g++ \
    git \


    libmpich-dev \
    libomp-dev \
    mpich \
    ninja-build \
    rsync \
    ssh \
    rocthrust-dev \
    && \
    apt clean



# system packaged version



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

# Build and install Kokkos
ARG KOKKOS_VERSION=3.7.01
COPY kokkos_cmake_config.cmake kokkos_cmake_config.cmake
RUN curl -L https://github.com/kokkos/kokkos/archive/refs/tags/$KOKKOS_VERSION.tar.gz | tar -xzf - && \
    cmake -S kokkos-$KOKKOS_VERSION -B build -C kokkos_cmake_config.cmake                          && \
    cmake --build build -v                                                                         && \
    cmake --install build                                                                          && \
    rm -rf build kokkos-$KOKKOS_VERSION
