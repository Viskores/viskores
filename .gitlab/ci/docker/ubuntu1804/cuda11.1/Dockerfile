FROM nvidia/cuda:11.6.1-devel-ubuntu18.04
LABEL maintainer "Robert Maynard<robert.maynard@kitware.com>"

# Base dependencies for building VTK-m projects
RUN apt-get update && apt-get install -y --no-install-recommends \
      curl \
      g++-8 \
      clang-8 \
      git \


      libmpich-dev \
      libomp-dev \
      libtbb-dev \
      mpich \
      ninja-build \
      && \
    rm -rf /var/lib/apt/lists/*



# system packaged version



# Provide a consistent CMake path across all images
# Allow tests that require CMake to work correctly
RUN mkdir /opt/cmake && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.16.7/cmake-3.16.7-Linux-x86_64.sh > cmake-3.16.7-Linux-x86_64.sh && \
    sh cmake-3.16.7-Linux-x86_64.sh --prefix=/opt/cmake/ --exclude-subdir --skip-license && \
    rm cmake-3.16.7-Linux-x86_64.sh

# Provide CMake 3.17 so we can re-run tests easily
# This will be used when we run just the tests
RUN mkdir /opt/cmake-latest/ && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.17.3/cmake-3.17.3-Linux-x86_64.sh > cmake-3.17.3-Linux-x86_64.sh && \
    sh cmake-3.17.3-Linux-x86_64.sh --prefix=/opt/cmake-latest/ --exclude-subdir --skip-license && \
    rm cmake-3.17.3-Linux-x86_64.sh && \
    ln -s /opt/cmake-latest/bin/ctest /opt/cmake-latest/bin/ctest-latest

ENV PATH "/opt/cmake/bin:/opt/cmake-latest/bin:${PATH}"
