FROM nvidia/cuda:10.2-devel-centos7
LABEL maintainer "Robert Maynard<robert.maynard@kitware.com>"

RUN yum install cmake make gcc gcc-c++ -y





# Provide a consistent CMake path across all images
# Install CMake 3.13 as it is the minium for cuda builds
RUN mkdir /opt/cmake && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.13.5/cmake-3.13.5-Linux-x86_64.sh > cmake-3.13.5-Linux-x86_64.sh && \
    sh cmake-3.13.5-Linux-x86_64.sh --prefix=/opt/cmake/ --exclude-subdir --skip-license

# Provide CMake 3.17 so we can re-run tests easily
# This will be used when we run just the tests
RUN mkdir /opt/cmake-latest/ && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.17.0/cmake-3.17.0-Linux-x86_64.sh > cmake-3.17.0-Linux-x86_64.sh && \
    sh cmake-3.17.0-Linux-x86_64.sh --prefix=/opt/cmake-latest/ --exclude-subdir --skip-license && \
    ln -s /opt/cmake-latest/bin/ctest /opt/cmake-latest/bin/ctest-latest

ENV PATH "/opt/cmake/bin:/opt/cmake-latest/bin:${PATH}"
