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

namespace
{

/// Deletes a pointer declared with the CUDA API
struct CudaDeleter
{
  VTKM_CONT void operator()(void* buffer) const
  {
    VTKM_ASSERT(buffer != nullptr);
    vtkm::cont::cuda::internal::CudaAllocator::Free(buffer);
  }
};

struct BufferInfoCuda : vtkm::cont::internal::BufferInfo
{
  std::shared_ptr<vtkm::UInt8> CudaBuffer;
  vtkm::BufferSizeType Size;

  VTKM_CONT BufferInfoCuda(const std::shared_ptr<vtkm::UInt8> buffer, vtkm::BufferSizeType size)
    : CudaBuffer(buffer)
    , Size(size)
  {
  }

  VTKM_CONT void* GetPointer() const override { return this->CudaBuffer.get(); }

  VTKM_CONT vtkm::BufferSizeType GetSize() const override { return this->Size; }
};

} // anonymous namespace

std::shared_ptr<vtkm::cont::internal::BufferInfo> vtkm::cont::internal::DeviceAdapterMemoryManager<
  vtkm::cont::DeviceAdapterTagCuda>::Allocate(vtkm::BufferSizeType size)
{
  try
  {
    vtkm::UInt8* buffer = static_cast<vtkm::UInt8*>(
      vtkm::cont::cuda::internal::CudaAllocator::Allocate(static_cast<std::size_t>(size)));
    std::shared_ptr<vtkm::UInt8> bufferPtr(buffer, CudaDeleter{});
    return std::shared_ptr<vtkm::cont::internal::BufferInfo>(new BufferInfoCuda(bufferPtr, size));
  }
  catch (const std::exception& error)
  {
    std::ostringstream err;
    err << "Failed to allocate " << size << " bytes on device: " << error.what();
    throw vtkm::cont::ErrorBadAllocation(err.str());
  }
}

std::shared_ptr<vtkm::cont::internal::BufferInfo> vtkm::cont::internal::DeviceAdapterMemoryManager<
  vtkm::cont::DeviceAdapterTagCuda>::ManageArray(std::shared_ptr<vtkm::UInt8> buffer,
                                                 vtkm::BufferSizeType size)
{
  return std::shared_ptr<vtkm::cont::internal::BufferInfo>(new BufferInfoCuda(buffer, size));
}

void vtkm::cont::internal::DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagCuda>::Reallocate(
  std::shared_ptr<vtkm::cont::internal::BufferInfo> b,
  vtkm::BufferSizeType newSize)
{
  BufferInfoCuda* buffer = dynamic_cast<BufferInfoCuda*>(b.get());
  VTKM_ASSERT(buffer);

  if (newSize <= buffer->Size)
  {
    // Just reuse the buffer. (Would be nice to free up memory.)
    buffer->Size = newSize;
  }
  else
  {
    // Make a new buffer
    std::shared_ptr<vtkm::cont::internal::BufferInfo> newBufferInfo = this->Allocate(newSize);
    BufferInfoCuda* newBuffer = dynamic_cast<BufferInfoCuda*>(newBufferInfo.get());
    VTKM_ASSERT(newBuffer != nullptr);

    // Copy the data to the new buffer
    VTKM_CUDA_CALL(cudaMemcpyAsync(newBuffer->GetPointer(),
                                   buffer->GetPointer(),
                                   static_cast<std::size_t>(buffer->Size),
                                   cudaMemcpyDeviceToDevice,
                                   cudaStreamPerThread));

    // Reset the buffer in the passed in info
    *buffer = *newBuffer;
  }
}

std::shared_ptr<vtkm::cont::internal::BufferInfo> vtkm::cont::internal::
  DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagCuda>::CopyHostToDevice(
    std::shared_ptr<vtkm::cont::internal::BufferInfoHost> src)
{
  using vtkm::cont::cuda::internal::CudaAllocator;
  if (CudaAllocator::IsManagedPointer(src->GetPointer()))
  {
    // In the current code structure, we don't know whether this buffer is going to be used
    // for input or output. (Currently, I don't think there is any difference.)
    CudaAllocator::PrepareForOutput(src->GetPointer(), static_cast<std::size_t>(src->GetSize()));

    // The provided control pointer is already cuda managed and can be accessed on the device
    // via unified memory. Just shallow copy the pointer.
    return std::shared_ptr<vtkm::cont::internal::BufferInfo>(
      new BufferInfoCuda(src->GetSharedPointer(), src->GetSize()));
  }
  else
  {
    // Make a new buffer
    std::shared_ptr<vtkm::cont::internal::BufferInfo> destInfo = this->Allocate(src->GetSize());
    BufferInfoCuda* dest = dynamic_cast<BufferInfoCuda*>(destInfo.get());
    VTKM_ASSERT(dest != nullptr);

    // Copy the data to the new buffer
    VTKM_LOG_F(vtkm::cont::LogLevel::MemTransfer,
               "Copying host --> CUDA dev: %s (%lld bytes)",
               vtkm::cont::GetHumanReadableSize(static_cast<std::size_t>(src->GetSize())).c_str(),
               src->GetSize());

    VTKM_CUDA_CALL(cudaMemcpyAsync(dest->GetPointer(),
                                   src->GetPointer(),
                                   static_cast<std::size_t>(src->GetSize()),
                                   cudaMemcpyHostToDevice,
                                   cudaStreamPerThread));

    return destInfo;
  }
}

std::shared_ptr<vtkm::cont::internal::BufferInfoHost> vtkm::cont::internal::
  DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagCuda>::CopyDeviceToHost(
    std::shared_ptr<vtkm::cont::internal::BufferInfo> src_)
{
  using vtkm::cont::cuda::internal::CudaAllocator;
  BufferInfoCuda* src = dynamic_cast<BufferInfoCuda*>(src_.get());
  VTKM_ASSERT(src != nullptr);

  std::shared_ptr<vtkm::cont::internal::BufferInfoHost> dest;

  if (CudaAllocator::IsManagedPointer(src->GetPointer()))
  {
    // The provided control pointer is already cuda managed and can be accessed on the host
    // via unified memory. Just shallow copy the pointer.
    CudaAllocator::PrepareForControl(src->GetPointer(), static_cast<std::size_t>(src->Size));
    dest.reset(new vtkm::cont::internal::BufferInfoHost(src->CudaBuffer, src->Size));
  }
  else
  {
    // Make a new buffer
    dest.reset(new vtkm::cont::internal::BufferInfoHost(src->Size));

    // Copy the data to the new buffer
    VTKM_LOG_F(vtkm::cont::LogLevel::MemTransfer,
               "Copying CUDA dev --> host: %s (%lld bytes)",
               vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(src->Size)).c_str(),
               src->Size);

    VTKM_CUDA_CALL(cudaMemcpyAsync(dest->GetPointer(),
                                   src->GetPointer(),
                                   static_cast<std::size_t>(src->Size),
                                   cudaMemcpyDeviceToHost,
                                   cudaStreamPerThread));
  }

  //In all cases we have possibly multiple async calls queued up in
  //our stream. We need to block on the copy back to control since
  //we don't wanting it accessing memory that hasn't finished
  //being used by the GPU
  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTagCuda>::Synchronize();

  return dest;
}

std::shared_ptr<vtkm::cont::internal::BufferInfo> vtkm::cont::internal::
  DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagCuda>::CopyDeviceToDevice(
    std::shared_ptr<vtkm::cont::internal::BufferInfo> src)
{
  std::shared_ptr<vtkm::cont::internal::BufferInfo> dest = this->Allocate(src->GetSize());
  VTKM_CUDA_CALL(cudaMemcpyAsync(dest->GetPointer(),
                                 src->GetPointer(),
                                 static_cast<std::size_t>(src->GetSize()),
                                 cudaMemcpyDeviceToDevice,
                                 cudaStreamPerThread));

  return dest;
}
