//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/cuda/internal/DeviceAdapterTimerImplementationCuda.h>

#include <vtkm/Types.h>
#include <vtkm/cont/cuda/ErrorCuda.h>

#include <cuda.h>

namespace vtkm
{
namespace cont
{

DeviceAdapterTimerImplementation<
  vtkm::cont::DeviceAdapterTagCuda>::DeviceAdapterTimerImplementation()
{
  VTKM_CUDA_CALL(cudaEventCreate(&this->StartEvent));
  VTKM_CUDA_CALL(cudaEventCreate(&this->StopEvent));
  this->Reset();
}

DeviceAdapterTimerImplementation<
  vtkm::cont::DeviceAdapterTagCuda>::~DeviceAdapterTimerImplementation()
{
  // These aren't wrapped in VTKM_CUDA_CALL because we can't throw errors
  // from destructors. We're relying on cudaGetLastError in the
  // VTKM_CUDA_CHECK_ASYNCHRONOUS_ERROR catching any issues from these calls
  // later.
  cudaEventDestroy(this->StartEvent);
  cudaEventDestroy(this->StopEvent);
}

void DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>::Reset()
{
  this->StartReady = false;
  this->StopReady = false;
}

void DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>::Start()
{
  VTKM_CUDA_CALL(cudaEventRecord(this->StartEvent, cudaStreamPerThread));
  this->StartReady = true;
}

void DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>::Stop()
{
  VTKM_CUDA_CALL(cudaEventRecord(this->StopEvent, cudaStreamPerThread));
  VTKM_CUDA_CALL(cudaEventSynchronize(this->StopEvent));
  this->StopReady = true;
}

bool DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>::Started() const
{
  return this->StartReady;
}

bool DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>::Stopped() const
{
  return this->StopReady;
}

// Callbacks without a mandated order(in independent streams) execute in undefined
// order and maybe serialized. So Instead CudaEventQuery is used here.
// Ref link: https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__STREAM.html
bool DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>::Ready() const
{
  if (cudaEventQuery(this->StopEvent) == cudaSuccess)
  {
    return true;
  }
  return false;
}


vtkm::Float64 DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>::GetElapsedTime()
  const
{
  assert(this->StartReady);
  if (!this->StartReady)
  {
    VTKM_LOG_F(vtkm::cont::LogLevel::Error,
               "Start() function should be called first then trying to call GetElapsedTime().");
    return 0;
  }
  if (!this->StopReady)
  {
    // Stop was not called, so we have to insert a new event into the stream
    VTKM_CUDA_CALL(cudaEventRecord(this->StopEvent, cudaStreamPerThread));
    VTKM_CUDA_CALL(cudaEventSynchronize(this->StopEvent));
  }

  float elapsedTimeMilliseconds;
  VTKM_CUDA_CALL(cudaEventElapsedTime(&elapsedTimeMilliseconds, this->StartEvent, this->StopEvent));
  // Reset Stop flag to its original state
  return static_cast<vtkm::Float64>(0.001f * elapsedTimeMilliseconds);
}
}
} // namespace vtkm::cont
