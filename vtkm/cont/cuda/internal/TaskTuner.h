//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_cont_cuda_internal_TaskTuner_h
#define vtk_m_cont_cuda_internal_TaskTuner_h

#include <vtkm/Types.h>
#include <vtkm/cont/cuda/ErrorCuda.h>

#include <cuda.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

template <class FunctorType>
__global__ void Schedule1DIndexKernel(FunctorType functor, vtkm::Id, vtkm::Id);
template <class FunctorType>
__global__ void Schedule1DIndexKernel2(FunctorType functor, vtkm::Id, vtkm::Id);
template <class FunctorType>
__global__ void Schedule3DIndexKernel(FunctorType functor, dim3 size);
template <class FunctorType>
__global__ void Schedule3DIndexKernel2(FunctorType functor, dim3 size);

void compute_block_size(dim3 rangeMax, dim3 blockSize3d, dim3& gridSize3d);


template <typename Task>
__global__ void TaskStrided1DLaunch(Task task, vtkm::Id size)
{
  const vtkm::Id start = static_cast<vtkm::Id>(blockIdx.x * blockDim.x + threadIdx.x);
  const vtkm::Id inc = static_cast<vtkm::Id>(blockDim.x * gridDim.x);
  for (vtkm::Id i = start; i < size; i += inc)
  {
    task(i);
  }
}

class PerfRecord
{
public:
  PerfRecord(float elapsedT, dim3 block)
    : elapsedTime(elapsedT)
    , blockSize(block)
  {
  }

  bool operator<(const PerfRecord& other) const { return elapsedTime < other.elapsedTime; }

  float elapsedTime;
  dim3 blockSize;
};

template <typename Task>
static void BlockSizeGuesser(vtkm::Id size, int& grids, int& blocks, float& occupancy)
{
  int blockSize;   // The launch configurator returned block size
  int minGridSize; // The minimum grid size needed to achieve the
                   // maximum occupancy for a full device launch
  int gridSize;    // The actual grid size needed, based on number of SM's
  int device;      // device to run on
  int numSMs;      // number of SMs on the active device

  cudaGetDevice(&device);
  cudaDeviceGetAttribute(&numSMs, cudaDevAttrMultiProcessorCount, device);

  cudaOccupancyMaxPotentialBlockSize(&minGridSize, &blockSize, Schedule1DIndexKernel2<Task>, 0, 0);


  blockSize /= (numSMs * 2);
  // Round up according to array size
  // gridSize = (size + blockSize - 1) / blockSize;
  gridSize = 32 * numSMs;
  // std::cout << "numSMs: " << numSMs << std::endl;

  // calculate theoretical occupancy
  int maxActiveBlocks;
  cudaOccupancyMaxActiveBlocksPerMultiprocessor(
    &maxActiveBlocks, Schedule1DIndexKernel2<Task>, blockSize, 0);

  cudaDeviceProp props;
  cudaGetDeviceProperties(&props, device);

  grids = gridSize;
  blocks = blockSize;
  occupancy = (maxActiveBlocks * blockSize / props.warpSize) /
    (float)(props.maxThreadsPerMultiProcessor / props.warpSize);
}

template <class Functor>
static void compare_1d_dynamic_block_picker(Functor functor,
                                            vtkm::Id size,
                                            const vtkm::Id& currentGridSize,
                                            const vtkm::Id& currentBlockSize)
{
  const std::type_info& ti = typeid(functor);
  std::cout << "fixed 1d block size performance " << ti.name() << std::endl;
  {
    cudaEvent_t start, stop;
    VTKM_CUDA_CALL(cudaEventCreate(&start));
    VTKM_CUDA_CALL(cudaEventCreate(&stop));

    VTKM_CUDA_CALL(cudaEventRecord(start, cudaStreamPerThread));
    Schedule1DIndexKernel<Functor><<<currentGridSize, currentBlockSize, 0, cudaStreamPerThread>>>(
      functor, vtkm::Id(0), size);
    VTKM_CUDA_CALL(cudaEventRecord(stop, cudaStreamPerThread));

    VTKM_CUDA_CALL(cudaEventSynchronize(stop));
    float elapsedTimeMilliseconds;
    VTKM_CUDA_CALL(cudaEventElapsedTime(&elapsedTimeMilliseconds, start, stop));

    VTKM_CUDA_CALL(cudaEventDestroy(start));
    VTKM_CUDA_CALL(cudaEventDestroy(stop));

    std::cout << "Schedule1DIndexKernel size: " << size << std::endl;
    std::cout << "GridSize of: " << currentGridSize << " BlockSize of: " << currentBlockSize
              << " required: " << elapsedTimeMilliseconds << std::endl;
  }

  std::cout << "dynamic 1d block size performance " << ti.name() << std::endl;
  {

    int grids, blocks;
    float occupancy;
    BlockSizeGuesser<Functor>(size, grids, blocks, occupancy);

    cudaEvent_t start, stop;
    VTKM_CUDA_CALL(cudaEventCreate(&start));
    VTKM_CUDA_CALL(cudaEventCreate(&stop));


    VTKM_CUDA_CALL(cudaEventRecord(start, cudaStreamPerThread));
    Schedule1DIndexKernel2<Functor><<<grids, blocks, 0, cudaStreamPerThread>>>(
      functor, vtkm::Id(0), size);
    VTKM_CUDA_CALL(cudaEventRecord(stop, cudaStreamPerThread));

    VTKM_CUDA_CALL(cudaEventSynchronize(stop));
    float elapsedTimeMilliseconds;
    VTKM_CUDA_CALL(cudaEventElapsedTime(&elapsedTimeMilliseconds, start, stop));

    VTKM_CUDA_CALL(cudaEventDestroy(start));
    VTKM_CUDA_CALL(cudaEventDestroy(stop));

    std::cout << "Schedule1DIndexKernel2 size: " << size << std::endl;
    std::cout << "GridSize of: " << grids << " BlockSize of: " << blocks
              << " required: " << elapsedTimeMilliseconds << std::endl;
  }
  std::cout << std::endl;
}

template <class Functor>
static void compare_3d_dynamic_block_picker(Functor functor,
                                            vtkm::Id3 ranges,
                                            const dim3& gridSize3d,
                                            const dim3& blockSize3d)
{
  const std::type_info& ti = typeid(functor);
  std::cout << "fixed 3d block size performance " << ti.name() << std::endl;
  {
    cudaEvent_t start, stop;
    VTKM_CUDA_CALL(cudaEventCreate(&start));
    VTKM_CUDA_CALL(cudaEventCreate(&stop));

    VTKM_CUDA_CALL(cudaEventRecord(start, cudaStreamPerThread));
    Schedule3DIndexKernel<Functor><<<gridSize3d, blockSize3d, 0, cudaStreamPerThread>>>(functor,
                                                                                        ranges);
    VTKM_CUDA_CALL(cudaEventRecord(stop, cudaStreamPerThread));

    VTKM_CUDA_CALL(cudaEventSynchronize(stop));
    float elapsedTimeMilliseconds;
    VTKM_CUDA_CALL(cudaEventElapsedTime(&elapsedTimeMilliseconds, start, stop));

    VTKM_CUDA_CALL(cudaEventDestroy(start));
    VTKM_CUDA_CALL(cudaEventDestroy(stop));

    // std::cout << "Schedule3DIndexKernel size: " << size << std::endl;
    // std::cout << "GridSize of: " << currentGridSize
    //           << " BlockSize of: " << currentBlockSize  << " required: " << elapsedTimeMilliseconds << std::endl;
  }

  std::cout << "dynamic 3d block size performance " << ti.name() << std::endl;
  {

    // int grids, blocks;
    // float occupancy;
    // BlockSizeGuesser<Functor>(size, grids, blocks, occupancy);

    // cudaEvent_t start, stop;
    // VTKM_CUDA_CALL(cudaEventCreate(&start));
    // VTKM_CUDA_CALL(cudaEventCreate(&stop));


    // VTKM_CUDA_CALL(cudaEventRecord(start, 0));
    // Schedule3DIndexKernel2<Functor><<<grids, blocks>>>(functor, vtkm::Id(0), size);
    // VTKM_CUDA_CALL(cudaEventRecord(stop, 0));

    // VTKM_CUDA_CALL(cudaEventSynchronize(stop));
    // float elapsedTimeMilliseconds;
    // VTKM_CUDA_CALL(cudaEventElapsedTime(&elapsedTimeMilliseconds, start, stop));

    // VTKM_CUDA_CALL(cudaEventDestroy(start));
    // VTKM_CUDA_CALL(cudaEventDestroy(stop));

    // std::cout << "Schedule3DIndexKernel2 size: " << size << std::endl;
    // std::cout << "GridSize of: " << grids
    //           << " BlockSize of: " << blocks  << " required: " << elapsedTimeMilliseconds << std::endl;
  }
  std::cout << std::endl;
}

template <class Functor>
static void parameter_sweep_3d_schedule(Functor functor, const vtkm::Id3& rangeMax)
{
  const dim3 ranges(static_cast<vtkm::UInt32>(rangeMax[0]),
                    static_cast<vtkm::UInt32>(rangeMax[1]),
                    static_cast<vtkm::UInt32>(rangeMax[2]));
  std::vector<PerfRecord> results;
  vtkm::UInt32 indexTable[16] = { 1, 2, 4, 8, 12, 16, 20, 24, 28, 30, 32, 64, 128, 256, 512, 1024 };

  for (vtkm::UInt32 i = 0; i < 16; i++)
  {
    for (vtkm::UInt32 j = 0; j < 16; j++)
    {
      for (vtkm::UInt32 k = 0; k < 16; k++)
      {
        cudaEvent_t start, stop;
        VTKM_CUDA_CALL(cudaEventCreate(&start));
        VTKM_CUDA_CALL(cudaEventCreate(&stop));

        dim3 blockSize3d(indexTable[i], indexTable[j], indexTable[k]);
        dim3 gridSize3d;

        if ((blockSize3d.x * blockSize3d.y * blockSize3d.z) >= 1024 ||
            (blockSize3d.x * blockSize3d.y * blockSize3d.z) <= 4 || blockSize3d.z >= 64)
        {
          //cuda can't handle more than 1024 threads per block
          //so don't try if we compute higher than that

          //also don't try stupidly low numbers

          //cuda can't handle more than 64 threads in the z direction
          continue;
        }

        compute_block_size(ranges, blockSize3d, gridSize3d);
        VTKM_CUDA_CALL(cudaEventRecord(start, cudaStreamPerThread));
        Schedule3DIndexKernel<Functor><<<gridSize3d, blockSize3d, 0, cudaStreamPerThread>>>(functor,
                                                                                            ranges);
        VTKM_CUDA_CALL(cudaEventRecord(stop, cudaStreamPerThread));

        VTKM_CUDA_CALL(cudaEventSynchronize(stop));
        float elapsedTimeMilliseconds;
        VTKM_CUDA_CALL(cudaEventElapsedTime(&elapsedTimeMilliseconds, start, stop));

        VTKM_CUDA_CALL(cudaEventDestroy(start));
        VTKM_CUDA_CALL(cudaEventDestroy(stop));

        PerfRecord record(elapsedTimeMilliseconds, blockSize3d);
        results.push_back(record);
      }
    }
  }

  std::sort(results.begin(), results.end());
  const vtkm::Int64 size = static_cast<vtkm::Int64>(results.size());
  for (vtkm::Int64 i = 1; i <= size; i++)
  {
    vtkm::UInt64 index = static_cast<vtkm::UInt64>(size - i);
    vtkm::UInt32 x = results[index].blockSize.x;
    vtkm::UInt32 y = results[index].blockSize.y;
    vtkm::UInt32 z = results[index].blockSize.z;
    float t = results[index].elapsedTime;

    std::cout << "BlockSize of: " << x << "," << y << "," << z << " required: " << t << std::endl;
  }

  std::cout << "fixed 3d block size performance " << std::endl;
  {
    cudaEvent_t start, stop;
    VTKM_CUDA_CALL(cudaEventCreate(&start));
    VTKM_CUDA_CALL(cudaEventCreate(&stop));

    dim3 blockSize3d(64, 2, 1);
    dim3 gridSize3d;

    compute_block_size(ranges, blockSize3d, gridSize3d);
    VTKM_CUDA_CALL(cudaEventRecord(start, cudaStreamPerThread));
    Schedule3DIndexKernel<Functor><<<gridSize3d, blockSize3d, 0, cudaStreamPerThread>>>(functor,
                                                                                        ranges);
    VTKM_CUDA_CALL(cudaEventRecord(stop, cudaStreamPerThread));

    VTKM_CUDA_CALL(cudaEventSynchronize(stop));
    float elapsedTimeMilliseconds;
    VTKM_CUDA_CALL(cudaEventElapsedTime(&elapsedTimeMilliseconds, start, stop));

    VTKM_CUDA_CALL(cudaEventDestroy(start));
    VTKM_CUDA_CALL(cudaEventDestroy(stop));

    std::cout << "BlockSize of: " << blockSize3d.x << "," << blockSize3d.y << "," << blockSize3d.z
              << " required: " << elapsedTimeMilliseconds << std::endl;
    std::cout << "GridSize of: " << gridSize3d.x << "," << gridSize3d.y << "," << gridSize3d.z
              << " required: " << elapsedTimeMilliseconds << std::endl;
  }
}
}
}
}
}

#endif
