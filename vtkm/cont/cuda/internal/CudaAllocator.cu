//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <cstdlib>
#include <mutex>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/cuda/ErrorCuda.h>
#include <vtkm/cont/cuda/internal/CudaAllocator.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/cuda/internal/RuntimeDeviceConfigurationCuda.h>
#define NO_VTKM_MANAGED_MEMORY "NO_VTKM_MANAGED_MEMORY"

#include <cstdlib>
#include <mutex>
#include <vector>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <cuda_runtime.h>
VTKM_THIRDPARTY_POST_INCLUDE

// These static vars are in an anon namespace to work around MSVC linker issues.
namespace
{
// Has CudaAllocator::Initialize been called by any thread?
static std::once_flag IsInitializedFlag;

// Used to keep track of whether the CUDA allocator has been initialized CUDA has not
// been finalized (since CUDA does not seem to track that for us).
static bool IsInitialized = false;

// Holds how VTK-m currently allocates memory.
// When VTK-m is initialized we set this based on the hardware support ( HardwareSupportsManagedMemory ).
// The user can explicitly disable managed memory through an enviornment variable
// or by calling a function on the CudaAllocator.
// Likewise managed memory can be re-enabled by calling a function on CudaAllocator
// if and only if the underlying hardware supports pageable managed memory
static bool ManagedMemoryEnabled = false;

// True if concurrent pagable managed memory is supported by the machines hardware.
static bool HardwareSupportsManagedMemory = false;

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

void CudaAllocator::ForceManagedMemoryOff()
{
  if (HardwareSupportsManagedMemory)
  {
    ManagedMemoryEnabled = false;
    VTKM_LOG_F(vtkm::cont::LogLevel::Info, "CudaAllocator disabling managed memory");
  }
  else
  {
    VTKM_LOG_F(
      vtkm::cont::LogLevel::Warn,
      "CudaAllocator trying to disable managed memory on hardware that doesn't support it");
  }
}

void CudaAllocator::ForceManagedMemoryOn()
{
  if (HardwareSupportsManagedMemory)
  {
    ManagedMemoryEnabled = true;
    VTKM_LOG_F(vtkm::cont::LogLevel::Info, "CudaAllocator enabling managed memory");
  }
  else
  {
    VTKM_LOG_F(vtkm::cont::LogLevel::Warn,
               "CudaAllocator trying to enable managed memory on hardware that doesn't support it");
  }
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
#if CUDART_VERSION >= 11030
  const auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  if (tracker.GetThreadFriendlyMemAlloc())
  {
    VTKM_CUDA_CALL(cudaMallocAsync(&ptr, numBytes, cudaStreamPerThread));
  }
  else
#endif
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
#if CUDART_VERSION >= 11030
  const auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  if (tracker.GetThreadFriendlyMemAlloc())
  {
    VTKM_CUDA_CALL(cudaMallocAsync(&ptr, numBytes, cudaStreamPerThread));
  }
  else
#endif
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

void CudaAllocator::Free(void* ptr)
{
  if (!IsInitialized)
  {
    // Since the data was successfully allocated, it is a fair assumption that the CUDA
    // runtime has been finalized and a global object is trying to destroy itself. Since
    // CUDA already cleaned up all memory for program exit, we can ignore this free.
    return;
  }

  VTKM_LOG_F(vtkm::cont::LogLevel::MemExec, "Freeing CUDA allocation at %p.", ptr);

#if CUDART_VERSION >= 11030
  const auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  if (tracker.GetThreadFriendlyMemAlloc())
  {
    VTKM_CUDA_CALL(cudaFreeAsync(ptr, cudaStreamPerThread));
  }
  else
#endif
  {
    VTKM_CUDA_CALL(cudaFree(ptr));
  }
}

void CudaAllocator::FreeDeferred(void* ptr, std::size_t numBytes)
{
  if (!IsInitialized)
  {
    // Since the data was successfully allocated, it is a fair assumption that the CUDA
    // runtime has been finalized and a global object is trying to destroy itself. Since
    // CUDA already cleaned up all memory for program exit, we can ignore this free.
    return;
  }

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
    // TODO these hints need to be benchmarked and adjusted once we start
    // sharing the pointers between cont/exec
    VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetAccessedBy, cudaCpuDeviceId));
    VTKM_CUDA_CALL(cudaMemPrefetchAsync(ptr, numBytes, cudaCpuDeviceId, cudaStreamPerThread));
  }
}

void CudaAllocator::PrepareForInput(const void* ptr, std::size_t numBytes)
{
  if (IsManagedPointer(ptr) && numBytes >= Threshold)
  {
    vtkm::Id dev;
    vtkm::cont::RuntimeDeviceInformation()
      .GetRuntimeConfiguration(vtkm::cont::DeviceAdapterTagCuda())
      .GetDeviceInstance(dev);
    // VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetPreferredLocation, dev));
    // VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetReadMostly, dev));
    VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetAccessedBy, dev));
    VTKM_CUDA_CALL(cudaMemPrefetchAsync(ptr, numBytes, dev, cudaStreamPerThread));
  }
}

void CudaAllocator::PrepareForOutput(const void* ptr, std::size_t numBytes)
{
  if (IsManagedPointer(ptr) && numBytes >= Threshold)
  {
    vtkm::Id dev;
    vtkm::cont::RuntimeDeviceInformation()
      .GetRuntimeConfiguration(vtkm::cont::DeviceAdapterTagCuda())
      .GetDeviceInstance(dev);
    // VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetPreferredLocation, dev));
    // VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseUnsetReadMostly, dev));
    VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetAccessedBy, dev));
    VTKM_CUDA_CALL(cudaMemPrefetchAsync(ptr, numBytes, dev, cudaStreamPerThread));
  }
}

void CudaAllocator::PrepareForInPlace(const void* ptr, std::size_t numBytes)
{
  if (IsManagedPointer(ptr) && numBytes >= Threshold)
  {
    vtkm::Id dev;
    vtkm::cont::RuntimeDeviceInformation()
      .GetRuntimeConfiguration(vtkm::cont::DeviceAdapterTagCuda())
      .GetDeviceInstance(dev);
    // VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetPreferredLocation, dev));
    // VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseUnsetReadMostly, dev));
    VTKM_CUDA_CALL(cudaMemAdvise(ptr, numBytes, cudaMemAdviseSetAccessedBy, dev));
    VTKM_CUDA_CALL(cudaMemPrefetchAsync(ptr, numBytes, dev, cudaStreamPerThread));
  }
}

void CudaAllocator::Initialize()
{
  std::call_once(IsInitializedFlag, []() {
    auto cudaDeviceConfig = dynamic_cast<
      vtkm::cont::internal::RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagCuda>&>(
      vtkm::cont::RuntimeDeviceInformation{}.GetRuntimeConfiguration(
        vtkm::cont::DeviceAdapterTagCuda()));
    vtkm::Id numDevices;
    cudaDeviceConfig.GetMaxDevices(numDevices);

    if (numDevices == 0)
    {
      return;
    }

    // Check all devices, use the feature set supported by all
    bool managedMemorySupported = true;
    std::vector<cudaDeviceProp> cudaProp;
    cudaDeviceConfig.GetCudaDeviceProp(cudaProp);
    for (int i = 0; i < numDevices && managedMemorySupported; ++i)
    {
      // We check for concurrentManagedAccess, as devices with only the
      // managedAccess property have extra synchronization requirements.
      managedMemorySupported = managedMemorySupported && cudaProp[i].concurrentManagedAccess;
    }

    HardwareSupportsManagedMemory = managedMemorySupported;
    ManagedMemoryEnabled = managedMemorySupported;

    VTKM_LOG_F(vtkm::cont::LogLevel::Info,
               "CudaAllocator hardware %s managed memory",
               HardwareSupportsManagedMemory ? "supports" : "doesn't support");

// Check if users want to disable managed memory
#pragma warning(push)
// getenv is not thread safe on windows but since it's inside a call_once block so
// it's fine to suppress the warning here.
#pragma warning(disable : 4996)
    const char* buf = std::getenv(NO_VTKM_MANAGED_MEMORY);
#pragma warning(pop)
    if (managedMemorySupported && buf != nullptr)
    { //only makes sense to disable managed memory if the hardware supports it
      //in the first place
      ManagedMemoryEnabled = false;
      VTKM_LOG_F(
        vtkm::cont::LogLevel::Info,
        "CudaAllocator disabling managed memory due to NO_VTKM_MANAGED_MEMORY env variable");
    }

    // CUDA does not give any indication of whether it is still running, but we have found from
    // experience that it finalizes itself during program termination. However, the user might
    // have their own objects being cleaned up during termination after CUDA. We need a flag
    // to catch if this happens after CUDA finalizes itself. We will set this flag to true now
    // and false on termination. Because we are creating the atexit call here (after CUDA must
    // have initialized itself), C++ will require our function that unsets the flag to happen
    // before CUDA finalizes.
    IsInitialized = true;
    std::atexit([]() { IsInitialized = false; });
  });
}
}
}
}
} // end namespace vtkm::cont::cuda::internal
