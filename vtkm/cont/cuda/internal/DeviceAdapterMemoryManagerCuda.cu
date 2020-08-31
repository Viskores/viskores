//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/cuda/ErrorCuda.h>
#include <vtkm/cont/cuda/internal/CudaAllocator.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterMemoryManagerCuda.h>

#include <vtkm/cont/ErrorBadAllocation.h>

#include <vtkm/Math.h>

namespace
{

void* CudaAllocate(vtkm::BufferSizeType size)
{
  try
  {
    return vtkm::cont::cuda::internal::CudaAllocator::Allocate(static_cast<std::size_t>(size));
  }
  catch (const std::exception& error)
  {
    std::ostringstream err;
    err << "Failed to allocate " << size << " bytes on CUDA device: " << error.what();
    throw vtkm::cont::ErrorBadAllocation(err.str());
  }
}

void CudaDelete(void* memory)
{
  if (memory != nullptr)
  {
    vtkm::cont::cuda::internal::CudaAllocator::Free(memory);
  }
};

void CudaReallocate(void*& memory,
                    void*& container,
                    vtkm::BufferSizeType oldSize,
                    vtkm::BufferSizeType newSize)
{
  VTKM_ASSERT(memory == container);

  if (newSize > oldSize)
  {
    // Make a new buffer
    void* newMemory = CudaAllocate(newSize);

    // Copy the data to the new buffer
    VTKM_CUDA_CALL(cudaMemcpyAsync(newMemory,
                                   memory,
                                   static_cast<std::size_t>(oldSize),
                                   cudaMemcpyDeviceToDevice,
                                   cudaStreamPerThread));

    // Reset the buffer in the passed in info
    memory = container = newMemory;
  }
  else
  {
    // Just reuse the buffer.
  }
}

} // anonymous namespace

namespace vtkm
{
namespace cont
{
namespace internal
{

vtkm::cont::internal::BufferInfo DeviceAdapterMemoryManager<
  vtkm::cont::DeviceAdapterTagCuda>::Allocate(vtkm::BufferSizeType size) const
{
  void* memory = CudaAllocate(size);
  return vtkm::cont::internal::BufferInfo(
    vtkm::cont::DeviceAdapterTagCuda{}, memory, memory, size, CudaDelete, CudaReallocate);
}

vtkm::cont::DeviceAdapterId
DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagCuda>::GetDevice() const
{
  return vtkm::cont::DeviceAdapterTagCuda{};
}

vtkm::cont::internal::BufferInfo
DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagCuda>::CopyHostToDevice(
  const vtkm::cont::internal::BufferInfo& src) const
{
  VTKM_ASSERT(src.GetDevice() == vtkm::cont::DeviceAdapterTagUndefined{});

  if (vtkm::cont::cuda::internal::CudaAllocator::IsManagedPointer(src.GetPointer()))
  {
    // In the current code structure, we don't know whether this buffer is going to be used
    // for input or output. (Currently, I don't think there is any difference.)
    vtkm::cont::cuda::internal::CudaAllocator::PrepareForOutput(
      src.GetPointer(), static_cast<std::size_t>(src.GetSize()));

    // The provided control pointer is already cuda managed and can be accessed on the device
    // via unified memory. Just shallow copy the pointer.
    return vtkm::cont::internal::BufferInfo(src, vtkm::cont::DeviceAdapterTagCuda{});
  }
  else
  {
    // Make a new buffer
    vtkm::cont::internal::BufferInfo dest = this->Allocate(src.GetSize());

    this->CopyHostToDevice(src, dest);

    return dest;
  }
}

void DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagCuda>::CopyHostToDevice(
  const vtkm::cont::internal::BufferInfo& src,
  const vtkm::cont::internal::BufferInfo& dest) const
{
  if (vtkm::cont::cuda::internal::CudaAllocator::IsManagedPointer(src.GetPointer()) &&
      src.GetPointer() == dest.GetPointer())
  {
    // In the current code structure, we don't know whether this buffer is going to be used
    // for input or output. (Currently, I don't think there is any difference.)
    vtkm::cont::cuda::internal::CudaAllocator::PrepareForOutput(
      src.GetPointer(), static_cast<std::size_t>(src.GetSize()));

    // The provided pointers are both cuda managed and the same, so the data are already
    // the same.
  }
  else
  {
    vtkm::BufferSizeType size = vtkm::Min(src.GetSize(), dest.GetSize());

    VTKM_LOG_F(vtkm::cont::LogLevel::MemTransfer,
               "Copying host --> CUDA dev: %s (%lld bytes)",
               vtkm::cont::GetHumanReadableSize(static_cast<std::size_t>(size)).c_str(),
               size);

    VTKM_CUDA_CALL(cudaMemcpyAsync(dest.GetPointer(),
                                   src.GetPointer(),
                                   static_cast<std::size_t>(size),
                                   cudaMemcpyHostToDevice,
                                   cudaStreamPerThread));
  }
}


vtkm::cont::internal::BufferInfo
DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagCuda>::CopyDeviceToHost(
  const vtkm::cont::internal::BufferInfo& src) const
{
  VTKM_ASSERT(src.GetDevice() == vtkm::cont::DeviceAdapterTagCuda{});

  vtkm::cont::internal::BufferInfo dest;

  if (vtkm::cont::cuda::internal::CudaAllocator::IsManagedPointer(src.GetPointer()))
  {
    // The provided control pointer is already cuda managed and can be accessed on the host
    // via unified memory. Just shallow copy the pointer.
    vtkm::cont::cuda::internal::CudaAllocator::PrepareForControl(
      src.GetPointer(), static_cast<std::size_t>(src.GetSize()));
    dest = vtkm::cont::internal::BufferInfo(src, vtkm::cont::DeviceAdapterTagUndefined{});

    //In all cases we have possibly multiple async calls queued up in
    //our stream. We need to block on the copy back to control since
    //we don't wanting it accessing memory that hasn't finished
    //being used by the GPU
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTagCuda>::Synchronize();
  }
  else
  {
    // Make a new buffer
    dest = vtkm::cont::internal::AllocateOnHost(src.GetSize());

    this->CopyDeviceToHost(src, dest);
  }

  return dest;
}

void DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagCuda>::CopyDeviceToHost(
  const vtkm::cont::internal::BufferInfo& src,
  const vtkm::cont::internal::BufferInfo& dest) const
{
  if (vtkm::cont::cuda::internal::CudaAllocator::IsManagedPointer(dest.GetPointer()) &&
      src.GetPointer() == dest.GetPointer())
  {
    // The provided pointers are both cuda managed and the same, so the data are already
    // the same.
  }
  else
  {
    vtkm::BufferSizeType size = vtkm::Min(src.GetSize(), dest.GetSize());

    VTKM_LOG_F(vtkm::cont::LogLevel::MemTransfer,
               "Copying CUDA dev --> host: %s (%lld bytes)",
               vtkm::cont::GetHumanReadableSize(static_cast<std::size_t>(size)).c_str(),
               size);

    VTKM_CUDA_CALL(cudaMemcpyAsync(dest.GetPointer(),
                                   src.GetPointer(),
                                   static_cast<std::size_t>(size),
                                   cudaMemcpyDeviceToHost,
                                   cudaStreamPerThread));
  }

  //In all cases we have possibly multiple async calls queued up in
  //our stream. We need to block on the copy back to control since
  //we don't wanting it accessing memory that hasn't finished
  //being used by the GPU
  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTagCuda>::Synchronize();
}

vtkm::cont::internal::BufferInfo
DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagCuda>::CopyDeviceToDevice(
  const vtkm::cont::internal::BufferInfo& src) const
{
  vtkm::cont::internal::BufferInfo dest = this->Allocate(src.GetSize());
  this->CopyDeviceToDevice(src, dest);

  return dest;
}

void DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagCuda>::CopyDeviceToDevice(
  const vtkm::cont::internal::BufferInfo& src,
  const vtkm::cont::internal::BufferInfo& dest) const
{
  VTKM_CUDA_CALL(cudaMemcpyAsync(dest.GetPointer(),
                                 src.GetPointer(),
                                 static_cast<std::size_t>(src.GetSize()),
                                 cudaMemcpyDeviceToDevice,
                                 cudaStreamPerThread));
}
}
}
} // namespace vtkm::cont::internal
