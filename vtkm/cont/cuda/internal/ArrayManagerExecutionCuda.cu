//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#define vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_cu

#include <vtkm/cont/cuda/internal/ArrayManagerExecutionCuda.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::ExecutionArrayInterfaceBasic(
  StorageBasicBase& storage)
  : Superclass(storage)
{
}

DeviceAdapterId ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::GetDeviceId() const
{
  return VTKM_DEVICE_ADAPTER_CUDA;
}

void ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::Allocate(TypelessExecutionArray& execArray,
                                                                  vtkm::Id numBytes) const
{
  if (execArray.Array != nullptr)
  {
    const vtkm::Id cap =
      static_cast<char*>(execArray.ArrayCapacity) - static_cast<char*>(execArray.Array);

    if (cap < numBytes)
    { // Current allocation too small -- free & realloc
      this->Free(execArray);
    }
    else
    { // Reuse buffer if possible:
      execArray.ArrayEnd = static_cast<char*>(execArray.Array) + numBytes;
      return;
    }
  }

  VTKM_ASSERT(execArray.Array == nullptr);

  // Attempt to allocate:
  try
  {
    char* tmp;
#ifdef VTKM_USE_UNIFIED_MEMORY
    int dev;
    VTKM_CUDA_CALL(cudaGetDevice(&dev));
    VTKM_CUDA_CALL(cudaMallocManaged(&tmp, static_cast<std::size_t>(numBytes)));
    VTKM_CUDA_CALL(cudaMemAdvise(tmp, numBytes, cudaMemAdviseSetPreferredLocation, dev));
    VTKM_CUDA_CALL(cudaMemPrefetchAsync(tmp, numBytes, dev, 0));
    VTKM_CUDA_CALL(cudaStreamSynchronize(0));
#else
    VTKM_CUDA_CALL(cudaMalloc(&tmp, static_cast<std::size_t>(numBytes)));
#endif

    execArray.Array = tmp;
    execArray.ArrayEnd = tmp + numBytes;
    execArray.ArrayCapacity = tmp + numBytes;
  }
  catch (const std::exception& error)
  {
    std::ostringstream err;
    err << "Failed to allocate " << numBytes << " bytes on device: " << error.what();
    throw vtkm::cont::ErrorBadAllocation(err.str());
  }
}

void ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::Free(
  TypelessExecutionArray& execArray) const
{
  if (execArray.Array != nullptr)
  {
    VTKM_CUDA_CALL(cudaFree(execArray.Array));
    execArray.Array = nullptr;
    execArray.ArrayEnd = nullptr;
    execArray.ArrayCapacity = nullptr;
  }
}

void ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::CopyFromControl(const void* controlPtr,
                                                                         void* executionPtr,
                                                                         vtkm::Id numBytes) const
{
  VTKM_CUDA_CALL(cudaMemcpy(
    executionPtr, controlPtr, static_cast<std::size_t>(numBytes), cudaMemcpyHostToDevice));
}

void ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::CopyToControl(const void* executionPtr,
                                                                       void* controlPtr,
                                                                       vtkm::Id numBytes) const
{
  VTKM_CUDA_CALL(cudaMemcpy(
    controlPtr, executionPtr, static_cast<std::size_t>(numBytes), cudaMemcpyDeviceToHost));
}

} // end namespace internal

VTKM_INSTANTIATE_ARRAYHANDLES_FOR_DEVICE_ADAPTER(DeviceAdapterTagCuda)
}
} // end vtkm::cont
