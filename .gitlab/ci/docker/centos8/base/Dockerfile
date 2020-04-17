FROM centos:8
LABEL maintainer "Robert Maynard<robert.maynard@kitware.com>"

RUN yum install make gcc gcc-c++ curl libasan libubsan libomp clang -y





# Provide CMake 3.17 so we can re-run tests easily
# This will be used when we run just the tests
RUN mkdir /opt/cmake-latest/ && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.17.0/cmake-3.17.0-Linux-x86_64.sh > cmake-3.17.0-Linux-x86_64.sh && \
    sh cmake-3.17.0-Linux-x86_64.sh --prefix=/opt/cmake-latest/ --exclude-subdir --skip-license && \
    ln -s /opt/cmake-latest/bin/ctest /opt/cmake-latest/bin/ctest-latest

# Provide a consistent CMake path across all images. Just use the lastest cmake
RUN mkdir -p /opt/cmake/bin && ln -s /opt/cmake-latest/bin/cmake /opt/cmake/bin/cmake

ENV PATH "/opt/cmake/bin:/opt/cmake-latest/bin:${PATH}"
