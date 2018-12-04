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

#include <cstdlib>
#include <mutex>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/cuda/ErrorCuda.h>
#include <vtkm/cont/cuda/internal/CudaAllocator.h>
#define NO_VTKM_MANAGED_MEMORY "NO_VTKM_MANAGED_MEMORY"

#include <mutex>
#include <vector>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <cuda_runtime.h>
VTKM_THIRDPARTY_POST_INCLUDE

// These static vars are in an anon namespace to work around MSVC linker issues.
namespace
{
#if CUDART_VERSION >= 8000
// Has CudaAllocator::Initialize been called by any thread?
static std::once_flag IsInitialized;
#endif

// True if concurrent pagable managed memory is not disabled by user via a system
// environment variable and all devices support it.
static bool ManagedMemoryEnabled = false;

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
  return ManagedMemoryEnabled;
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
  if (!ptr || !ManagedMemoryEnabled)
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
#if CUDART_VERSION < 10000 // isManaged deprecated in CUDA 10.
  return attr.isManaged != 0;
#else // attr.type doesn't exist before CUDA 10
  return attr.type == cudaMemoryTypeManaged;
#endif
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
  if (ManagedMemoryEnabled)
  {
    VTKM_CUDA_CALL(cudaMallocManaged(&ptr, numBytes));
  }
  else
  {
    VTKM_CUDA_CALL(cudaMalloc(&ptr, numBytes));
  }

  {
    VTKM_LOG_F(vtkm::cont::LogLevel::MemExec,
               "Allocated CUDA array of %s at %p.",
               vtkm::cont::GetSizeString(numBytes).c_str(),
               ptr);
  }

  return ptr;
}

void* CudaAllocator::AllocateUnManaged(std::size_t numBytes)
{
  void* ptr = nullptr;
  VTKM_CUDA_CALL(cudaMalloc(&ptr, numBytes));
  {
    VTKM_LOG_F(vtkm::cont::LogLevel::MemExec,
               "Allocated CUDA array of %s at %p.",
               vtkm::cont::GetSizeString(numBytes).c_str(),
               ptr);
  }
  return ptr;
}

void CudaAllocator::Free(void* ptr)
{
  VTKM_LOG_F(vtkm::cont::LogLevel::MemExec, "Freeing CUDA allocation at %p.", ptr);
  VTKM_CUDA_CALL(cudaFree(ptr));
}

void CudaAllocator::FreeDeferred(void* ptr, std::size_t numBytes)
{
  static std::mutex deferredMutex;
  static std::vector<void*> deferredPointers;
  static std::size_t deferredSize = 0;
  constexpr std::size_t bufferLimit = 2 << 24; //16MB buffer

  {
    VTKM_LOG_F(vtkm::cont::LogLevel::MemExec,
               "Deferring free of CUDA allocation at %p of %s.",
               ptr,
               vtkm::cont::GetSizeString(numBytes).c_str());
  }

  std::vector<void*> toFree;
  // critical section
  {
    std::lock_guard<std::mutex> lock(deferredMutex);
    deferredPointers.push_back(ptr);
    deferredSize += numBytes;
    if (deferredSize >= bufferLimit)
    {
      toFree.swap(deferredPointers);
      deferredSize = 0;
    }
  }

  for (auto&& p : toFree)
  {
    VTKM_LOG_F(vtkm::cont::LogLevel::MemExec, "Freeing deferred CUDA allocation at %p.", p);
    VTKM_CUDA_CALL(cudaFree(p));
  }
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
  std::call_once(IsInitialized, []() {
    bool managedMemorySupported = true;
    int numDevices;
    VTKM_CUDA_CALL(cudaGetDeviceCount(&numDevices));

    if (numDevices == 0)
    {
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

    managedMemorySupported = managed;

// Check if users want to disable managed memory
#pragma warning(push)
// getenv is not thread safe on windows but since it's inside a call_once block so
// it's fine to suppress the warning here.
#pragma warning(disable : 4996)
    const char* buf = std::getenv(NO_VTKM_MANAGED_MEMORY);
#pragma warning(pop)
    if (buf != nullptr)
    {
      ManagedMemoryEnabled = (std::string(buf) != "1");
    }
    ManagedMemoryEnabled = ManagedMemoryEnabled && managedMemorySupported;
  });
#endif
}
}
}
}
} // end namespace vtkm::cont::cuda::internal
