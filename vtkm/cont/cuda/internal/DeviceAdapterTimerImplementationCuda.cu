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
  VTKM_CUDA_CALL(cudaEventCreate(&this->EndEvent));
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
  cudaEventDestroy(this->EndEvent);
}

void DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>::Reset()
{
  VTKM_CUDA_CALL(cudaEventRecord(this->StartEvent, cudaStreamPerThread));
  VTKM_CUDA_CALL(cudaEventSynchronize(this->StartEvent));
}

vtkm::Float64 DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>::GetElapsedTime()
{
  VTKM_CUDA_CALL(cudaEventRecord(this->EndEvent, cudaStreamPerThread));
  VTKM_CUDA_CALL(cudaEventSynchronize(this->EndEvent));
  float elapsedTimeMilliseconds;
  VTKM_CUDA_CALL(cudaEventElapsedTime(&elapsedTimeMilliseconds, this->StartEvent, this->EndEvent));
  return static_cast<vtkm::Float64>(0.001f * elapsedTimeMilliseconds);
}
}
} // namespace vtkm::cont
