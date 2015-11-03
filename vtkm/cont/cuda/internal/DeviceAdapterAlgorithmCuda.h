//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h
#define vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/cuda/internal/ArrayManagerExecutionCuda.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorControlInternal.h>

// Here are the actual implementation of the algorithms.
#include <vtkm/cont/cuda/internal/DeviceAdapterAlgorithmThrust.h>

#include <cuda.h>

namespace vtkm {
namespace cont {

template<>
struct DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>
    : public vtkm::cont::cuda::internal::DeviceAdapterAlgorithmThrust<
          vtkm::cont::DeviceAdapterTagCuda>
{

  VTKM_CONT_EXPORT static void Synchronize()
  {
    cudaError_t error = cudaDeviceSynchronize();
    if (error != cudaSuccess)
    {
      throw vtkm::cont::ErrorControlInternal(cudaGetErrorString(error));
    }
  }

};

/// CUDA contains its own high resolution timer.
///
template<>
class DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>
{
public:
  VTKM_CONT_EXPORT DeviceAdapterTimerImplementation()
  {
    cudaEventCreate(&this->StartEvent);
    cudaEventCreate(&this->EndEvent);
    this->Reset();
  }
  VTKM_CONT_EXPORT ~DeviceAdapterTimerImplementation()
  {
    cudaEventDestroy(this->StartEvent);
    cudaEventDestroy(this->EndEvent);
  }

  VTKM_CONT_EXPORT void Reset()
  {
    cudaEventRecord(this->StartEvent, 0);
  }

  VTKM_CONT_EXPORT vtkm::Float64 GetElapsedTime()
  {
    cudaEventRecord(this->EndEvent, 0);
    cudaEventSynchronize(this->EndEvent);
    float elapsedTimeMilliseconds;
    cudaEventElapsedTime(&elapsedTimeMilliseconds,
                         this->StartEvent,
                         this->EndEvent);
    return static_cast<vtkm::Float64>(0.001f*elapsedTimeMilliseconds);
  }

private:
  // Copying CUDA events is problematic.
  DeviceAdapterTimerImplementation(const DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda> &);
  void operator=(const DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda> &);

  cudaEvent_t StartEvent;
  cudaEvent_t EndEvent;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h
