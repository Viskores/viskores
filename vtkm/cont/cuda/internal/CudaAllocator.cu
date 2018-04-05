//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/cuda/ErrorCuda.h>
#include <vtkm/cont/cuda/internal/CudaAllocator.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <cuda_runtime.h>
VTKM_THIRDPARTY_POST_INCLUDE

// These static vars are in an anon namespace to work around MSVC linker issues.
namespace
{
#if CUDART_VERSION >= 8000
// Has CudaAllocator::Initialize been called?
static bool IsInitialized = false;
#endif

// True if all devices support concurrent pagable managed memory.
static bool ManagedMemorySupported = false;

// Avoid overhead of cudaMemAdvise and cudaMemPrefetchAsync for small buffers.
// This value should be > 0 or else these functions will error out.
static std::size_t Threshold = 1 << 20;
}

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

bool CudaAllocator::UsingManagedMemory()
{
  CudaAllocator::Initialize();
  return ManagedMemorySupported;
}

bool CudaAllocator::IsDevicePointer(const void* ptr)
{
  CudaAllocator::Initialize();
  if (!ptr)
  {
    return false;
  }

  cudaPointerAttributes attr;
  cudaError_t err = cudaPointerGetAttributes(&attr, ptr);
  // This function will return invalid value if the pointer is unknown to the
  // cuda runtime. Manually catch this value since it's not really an error.
  if (err == cudaErrorInvalidValue)
  {
    cudaGetLastError(); // Clear the error so we don't raise it later...
    return false;
  }
  VTKM_CUDA_CALL(err /*= cudaPointerGetAttributes(&attr, ptr)*/);
  return attr.devicePointer == ptr;
}

bool CudaAllocator::IsManagedPointer(const void* ptr)
{
  if (!ptr || !ManagedMemorySupported)
  {
    return false;
  }

  cudaPointerAttributes attr;
  cudaError_t err = cudaPointerGetAttributes(&attr, ptr);
  // This function will return invalid value if the pointer is unknown to the
  // cuda runtime. Manually catch this value since it's not really an error.
  if (err == cudaErrorInvalidValue)
  {
    cudaGetLastError(); // Clear the error so we don't raise it later...
    return false;
  }
  VTKM_CUDA_CALL(err /*= cudaPointerGetAttributes(&attr, ptr)*/);
  return attr.isManaged != 0;
}

void* CudaAllocator::Allocate(std::size_t numBytes)
{
  CudaAllocator::Initialize();
  // When numBytes is zero cudaMallocManaged returns an error and the behavior
  // of cudaMalloc is not documented. Just return nullptr.
  if (numBytes == 0)
  {
    return nullptr;
  }

  void* ptr = nullptr;
  if (ManagedMemorySupported)
  {
    VTKM_CUDA_CALL(cudaMallocManaged(&ptr, numBytes));
  }
  else
  {
    VTKM_CUDA_CALL(cudaMalloc(&ptr, numBytes));
  }

  return ptr;
}

void CudaAllocator::Free(void* ptr)
{
  VTKM_CUDA_CALL(cudaFree(ptr));
}

void CudaAllocator::PrepareForControl(const void* ptr, std::size_t numBytes)
{
  if (IsManagedPointer(ptr) && numBytes >= Threshold)
  {
#if CUDART_VERSION >= 8000
    // TODO these hints need to be benchmarked and adjusted once we start
    // sharing the pointers between cont/exec
    VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetAccessedBy, cudaCpuDeviceId));
    VTKM_CUDA_CALL(cudaMemPrefetchAsync(ptr, numBytes, cudaCpuDeviceId, cudaStreamPerThread));
#endif // CUDA >= 8.0
  }
}

void CudaAllocator::PrepareForInput(const void* ptr, std::size_t numBytes)
{
  if (IsManagedPointer(ptr) && numBytes >= Threshold)
  {
#if CUDART_VERSION >= 8000
    int dev;
    VTKM_CUDA_CALL(cudaGetDevice(&dev));
    // VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetPreferredLocation, dev));
    // VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetReadMostly, dev));
    VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetAccessedBy, dev));
    VTKM_CUDA_CALL(cudaMemPrefetchAsync(ptr, numBytes, dev, cudaStreamPerThread));
#endif // CUDA >= 8.0
  }
}

void CudaAllocator::PrepareForOutput(const void* ptr, std::size_t numBytes)
{
  if (IsManagedPointer(ptr) && numBytes >= Threshold)
  {
#if CUDART_VERSION >= 8000
    int dev;
    VTKM_CUDA_CALL(cudaGetDevice(&dev));
    // VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetPreferredLocation, dev));
    // VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseUnsetReadMostly, dev));
    VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetAccessedBy, dev));
    VTKM_CUDA_CALL(cudaMemPrefetchAsync(ptr, numBytes, dev, cudaStreamPerThread));
#endif // CUDA >= 8.0
  }
}

void CudaAllocator::PrepareForInPlace(const void* ptr, std::size_t numBytes)
{
  if (IsManagedPointer(ptr) && numBytes >= Threshold)
  {
#if CUDART_VERSION >= 8000
    int dev;
    VTKM_CUDA_CALL(cudaGetDevice(&dev));
    // VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetPreferredLocation, dev));
    // VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseUnsetReadMostly, dev));
    VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetAccessedBy, dev));
    VTKM_CUDA_CALL(cudaMemPrefetchAsync(ptr, numBytes, dev, cudaStreamPerThread));
#endif // CUDA >= 8.0
  }
}

void CudaAllocator::Initialize()
{
#if CUDART_VERSION >= 8000
  if (!IsInitialized)
  {
    int numDevices;
    VTKM_CUDA_CALL(cudaGetDeviceCount(&numDevices));

    if (numDevices == 0)
    {
      ManagedMemorySupported = false;
      IsInitialized = true;
      return;
    }

    // Check all devices, use the feature set supported by all
    bool managed = true;
    cudaDeviceProp prop;
    for (int i = 0; i < numDevices && managed; ++i)
    {
      VTKM_CUDA_CALL(cudaGetDeviceProperties(&prop, i));
      // We check for concurrentManagedAccess, as devices with only the
      // managedAccess property have extra synchronization requirements.
      managed = managed && prop.concurrentManagedAccess;
    }

    ManagedMemorySupported = managed;
    IsInitialized = true;
  }
#else
  ManagedMemorySupported = false;
#endif
}
}
}
}
} // end namespace vtkm::cont::cuda::internal
