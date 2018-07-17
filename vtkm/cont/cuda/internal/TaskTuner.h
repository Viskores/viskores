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
#include <vector>

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

vtkm::UInt32 getNumSMs(int dId);

template <typename TaskType>
__global__ void TaskStrided1DLaunch(TaskType task, vtkm::Id);
template <typename TaskType>
__global__ void TaskStrided3DLaunch(TaskType task, dim3 size);

struct PerfRecord1d
{
  PerfRecord1d(float elapsedT, int g, int b)
    : elapsedTime(elapsedT)
    , grid(g)
    , block(b)
  {
  }

  bool operator<(const PerfRecord1d& other) const { return elapsedTime < other.elapsedTime; }

  float elapsedTime;
  int grid;
  int block;
};

inline std::ostream& operator<<(std::ostream& os, const PerfRecord1d& record)
{
  os << "TaskStrided1DLaunch<<<" << record.grid << "," << record.block
     << ">>> required: " << record.elapsedTime << "\n";
  return os;
}


struct PerfRecord3d
{
  PerfRecord3d(float elapsedT, int g, dim3 b)
    : elapsedTime(elapsedT)
    , grid(g)
    , block(b)
  {
  }

  bool operator<(const PerfRecord3d& other) const { return elapsedTime < other.elapsedTime; }

  float elapsedTime;
  int grid;
  dim3 block;
};

inline std::ostream& operator<<(std::ostream& os, const PerfRecord3d& record)
{

  os << "TaskStrided3DLaunch<<<" << record.grid << ",(" << record.block.x << "," << record.block.y
     << "," << record.block.z << ")>>> required: " << record.elapsedTime << "\n";
  return os;
}


template <typename TaskT>
static void parameter_sweep_1d_schedule(const TaskT& task, const vtkm::Id& numInstances)
{
  std::vector<PerfRecord1d> results;
  constexpr vtkm::UInt32 gridIndexTable[12] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048 };
  constexpr vtkm::UInt32 blockIndexTable[12] = { 4,   8,   16,   32,   64,   128,
                                                 256, 512, 1024, 2048, 4096, 8192 };

  int deviceId;
  VTKM_CUDA_CALL(cudaGetDevice(&deviceId)); //get deviceid from cuda

  for (vtkm::UInt32 g = 0; g < 12; g++)
  {
    vtkm::UInt32 grids = gridIndexTable[g] * getNumSMs(deviceId);
    for (vtkm::UInt32 b = 0; b < 12; b++)
    {
      vtkm::UInt32 blocks = blockIndexTable[b];

      cudaEvent_t start, stop;
      VTKM_CUDA_CALL(cudaEventCreate(&start));
      VTKM_CUDA_CALL(cudaEventCreate(&stop));

      TaskStrided1DLaunch<<<grids, blocks, 0, cudaStreamPerThread>>>(task, numInstances);

      VTKM_CUDA_CALL(cudaEventRecord(stop, cudaStreamPerThread));

      VTKM_CUDA_CALL(cudaEventSynchronize(stop));
      float elapsedTimeMilliseconds;
      VTKM_CUDA_CALL(cudaEventElapsedTime(&elapsedTimeMilliseconds, start, stop));

      VTKM_CUDA_CALL(cudaEventDestroy(start));
      VTKM_CUDA_CALL(cudaEventDestroy(stop));

      results.emplace_back(elapsedTimeMilliseconds, grids, blocks);
    }
  }

  std::sort(results.begin(), results.end());
  for (auto&& i : results)
  {
    std::cout << i << std::endl;
  }
}

template <typename TaskT>
static void parameter_sweep_3d_schedule(const TaskT& task, const vtkm::Id3& rangeMax)
{
  const dim3 ranges(static_cast<vtkm::UInt32>(rangeMax[0]),
                    static_cast<vtkm::UInt32>(rangeMax[1]),
                    static_cast<vtkm::UInt32>(rangeMax[2]));
  std::vector<PerfRecord3d> results;

  constexpr vtkm::UInt32 gridIndexTable[12] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048 };
  constexpr vtkm::UInt32 blockIndexTable[16] = { 1,  2,  4,  8,  12,  16,  20,  24,
                                                 28, 30, 32, 64, 128, 256, 512, 1024 };

  int deviceId;
  for (vtkm::UInt32 g = 0; g < 12; g++)
  {
    vtkm::UInt32 grids = gridIndexTable[g] * getNumSMs(deviceId);
    for (vtkm::UInt32 i = 0; i < 16; i++)
    {
      for (vtkm::UInt32 j = 0; j < 16; j++)
      {
        for (vtkm::UInt32 k = 0; k < 16; k++)
        {
          cudaEvent_t start, stop;
          VTKM_CUDA_CALL(cudaEventCreate(&start));
          VTKM_CUDA_CALL(cudaEventCreate(&stop));

          dim3 blocks(blockIndexTable[i], blockIndexTable[j], blockIndexTable[k]);

          if ((blocks.x * blocks.y * blocks.z) >= 1024 || (blocks.x * blocks.y * blocks.z) <= 4 ||
              blocks.z >= 64)
          {
            //cuda can't handle more than 1024 threads per block
            //so don't try if we compute higher than that

            //also don't try stupidly low numbers

            //cuda can't handle more than 64 threads in the z direction
            continue;
          }

          VTKM_CUDA_CALL(cudaEventRecord(start, cudaStreamPerThread));
          TaskStrided3DLaunch<<<grids, blocks, 0, cudaStreamPerThread>>>(task, ranges);
          VTKM_CUDA_CALL(cudaEventRecord(stop, cudaStreamPerThread));

          VTKM_CUDA_CALL(cudaEventSynchronize(stop));
          float elapsedTimeMilliseconds;
          VTKM_CUDA_CALL(cudaEventElapsedTime(&elapsedTimeMilliseconds, start, stop));

          VTKM_CUDA_CALL(cudaEventDestroy(start));
          VTKM_CUDA_CALL(cudaEventDestroy(stop));

          results.emplace_back(elapsedTimeMilliseconds, grids, blocks);
        }
      }
    }
  }

  std::sort(results.begin(), results.end());
  for (auto&& i : results)
  {
    std::cout << i << std::endl;
  }
}
}
}
}
}

#endif
