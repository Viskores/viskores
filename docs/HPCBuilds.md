# Building VTK-m on HPC systems #

Instructions for building VTK-m on various HPC systems is included below.


## Spock ##
Spock is a system at the OLCF that is an early access system for Frontier.
As the software stack on Spock is frequently updated, these directions may be updated from time to time.

Building VTK-m with HIP support for the AMD GPUs with the correct versions of Rocm, Kokkos and a patch to CMake can be done with the following script.


```sh
#!/bin/bash

set -x
set -e

module load rocm/4.3.0
module load cmake
module load gcc


hipcc_path=$(which hipcc)
rocm_path=$(dirname $hipcc_path)/../

home_dir=$(pwd)
cmake_src_dir=${home_dir}/cmake/src
cmake_build_dir=${home_dir}/cmake/build

if true; then
git clone -b add_hip_language https://gitlab.kitware.com/cjy7117/cmake.git ${cmake_src_dir}
rm -rf ${cmake_build_dir}
cmake -S ${cmake_src_dir} -B ${cmake_build_dir} \
			-DCMAKE_C_COMPILER=gcc \
      -DCMAKE_CXX_COMPILER=g++
cmake --build ${cmake_build_dir} -j10
fi

kokkos_src_dir=${home_dir}/kokkos/src
kokkos_build_dir=${home_dir}/kokkos/build
kokkos_install_dir=${home_dir}/kokkos/install

if true; then
rm -rf ${kokkos_src_dir}
git clone -b 3.4.01 https://github.com/kokkos/kokkos.git ${kokkos_src_dir}
rm -rf ${kokkos_build_dir}
${cmake_build_dir}/bin/cmake -S ${kokkos_src_dir} -B ${kokkos_build_dir} \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON\
  -DKokkos_ARCH_VEGA906=ON \
  -DCMAKE_CXX_COMPILER=${rocm_path}/bin/hipcc \
  -DKokkos_ENABLE_HIP=ON \
  -DKokkos_ENABLE_SERIAL=ON \
  -DKokkos_ENABLE_HIP_RELOCATABLE_DEVICE_CODE=OFF \
  -DCMAKE_INSTALL_PREFIX=${kokkos_install_dir} \
  -DCMAKE_CXX_FLAGS="--amdgpu-target=gfx908"
${cmake_build_dir}/bin/cmake --build ${kokkos_build_dir} -j10
${cmake_build_dir}/bin/cmake --install ${kokkos_build_dir}
fi


benchmark_src_dir=${home_dir}/benchmark/src
benchmark_build_dir=${home_dir}/benchmark/build
benchmark_install_dir=${home_dir}/benchmark/install

# build google benchmark only if you build vtk-m with benchmark ON
if true; then
curl --insecure -OL https://github.com/google/benchmark/archive/v1.5.2.tar.gz
mkdir -p benchmark && tar xf v1.5.2.tar.gz && mv benchmark-1.5.2 ${benchmark_src_dir}
rm -rf ${benchmark_build_dir}
${cmake_build_dir}/bin/cmake -S ${benchmark_src_dir} -B ${benchmark_build_dir}\
														 -DBENCHMARK_DOWNLOAD_DEPENDENCIES=ON\
                             -DCMAKE_C_COMPILER=gcc \
                             -DCMAKE_CXX_COMPILER=g++
${cmake_build_dir}/bin/cmake --build ${benchmark_build_dir} -j10
${cmake_build_dir}/bin/cmake --install ${benchmark_build_dir} --prefix ${benchmark_install_dir}
fi

vtkm_src_dir=${home_dir}/vtkm/src
vtkm_build_dir=${home_dir}/vtkm/build
vtkm_install_dir=${home_dir}/vtkm/install

if true; then
git clone -b master https://gitlab.kitware.com/vtk/vtk-m.git ${vtkm_src_dir}


rm -rf ${vtkm_build_dir}
${cmake_build_dir}/bin/cmake -S ${vtkm_src_dir} -B ${vtkm_build_dir} \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF\
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON\
  -DVTKm_NO_DEPRECATED_VIRTUAL=ON \
  -DVTKm_ENABLE_KOKKOS=ON \
  -DVTKm_ENABLE_MPI=OFF\
  -DVTKm_ENABLE_RENDERING=ON \
  -DVTKm_ENABLE_TESTING=OFF \
  -DVTKm_ENABLE_BENCHMARKS=ON\
  -DCMAKE_HIP_ARCHITECTURES="gfx908" \
  -DCMAKE_PREFIX_PATH="${kokkos_install_dir};${benchmark_install_dir}" \
  -DCMAKE_INSTALL_PREFIX=${vtkm_install_dir} \
  -DCMAKE_HIP_COMPILER_TOOLKIT_ROOT=${rocm_path}\
  -DCMAKE_CXX_COMPILER=${rocm_path}/llvm/bin/clang++ \
  -DCMAKE_C_COMPILER=${rocm_path}/llvm/bin/clang

${cmake_build_dir}/bin/cmake --build ${vtkm_build_dir} -j10
${cmake_build_dir}/bin/cmake --install ${vtkm_build_dir}
fi
```
