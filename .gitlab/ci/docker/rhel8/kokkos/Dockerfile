FROM nvidia/cuda:10.2-devel-ubi8
LABEL maintainer "Sujin Philip<sujin.philip@kitware.com>"

RUN yum install make gcc gcc-c++ curl -y





# kokkos backend requires cmake 3.18
RUN mkdir /opt/cmake/ && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.18.0/cmake-3.18.0-Linux-x86_64.sh > cmake-3.18.0-Linux-x86_64.sh && \
    sh cmake-3.18.0-Linux-x86_64.sh --prefix=/opt/cmake/ --exclude-subdir --skip-license && \
    rm cmake-3.18.0-Linux-x86_64.sh && \
    ln -s /opt/cmake/bin/ctest /opt/cmake/bin/ctest-latest

ENV PATH "/opt/cmake/bin:${PATH}"

# Build and install Kokkos
RUN mkdir -p /opt/kokkos/build && \
    cd /opt/kokkos/build && \
    curl -L https://github.com/kokkos/kokkos/archive/3.1.01.tar.gz > kokkos-3.1.01.tar.gz && \
    tar -xf kokkos-3.1.01.tar.gz && \
    mkdir bld && cd bld && \
    CXX=/opt/kokkos/build/kokkos-3.1.01/bin/nvcc_wrapper \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/kokkos -DCMAKE_CXX_FLAGS=-fPIC \
          -DKokkos_ENABLE_CUDA=ON -DKokkos_ENABLE_CUDA_CONSTEXPR=ON \
          -DKokkos_ENABLE_CUDA_LAMBDA=ON -DKokkos_ENABLE_CUDA_LDG_INTRINSIC=ON \
          -DKokkos_ENABLE_CUDA_RELOCATABLE_DEVICE_CODE=ON -DKokkos_ENABLE_CUDA_UVM=ON \
          -DKokkos_ARCH_TURING75=ON ../kokkos-3.1.01 && \
    make -j all && \
    make install
