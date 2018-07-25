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
#include <vtkm/cont/cuda/internal/CudaAllocator.h>
#include <vtkm/cont/cuda/internal/ExecutionArrayInterfaceBasicCuda.h>

using vtkm::cont::cuda::internal::CudaAllocator;

namespace vtkm
{
namespace cont
{
namespace internal
{

DeviceAdapterId ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::GetDeviceId() const
{
  return DeviceAdapterId(VTKM_DEVICE_ADAPTER_CUDA);
}

void ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::Allocate(TypelessExecutionArray& execArray,
                                                                  vtkm::Id numberOfValues,
                                                                  vtkm::UInt64 sizeOfValue) const
{
  const vtkm::UInt64 numBytes = static_cast<vtkm::UInt64>(numberOfValues) * sizeOfValue;
  // Detect if we can reuse a device-accessible pointer from the control env:
  if (CudaAllocator::IsDevicePointer(execArray.ArrayControl))
  {
    const vtkm::UInt64 managedCapacity =
      static_cast<vtkm::UInt64>(static_cast<const char*>(execArray.ArrayControlCapacity) -
                                static_cast<const char*>(execArray.ArrayControl));
    if (managedCapacity >= numBytes)
    {
      if (execArray.Array && execArray.Array != execArray.ArrayControl)
      {
        this->Free(execArray);
      }

      execArray.Array = const_cast<void*>(execArray.ArrayControl);
      execArray.ArrayEnd = static_cast<char*>(execArray.Array) + numBytes;
      execArray.ArrayCapacity = const_cast<void*>(execArray.ArrayControlCapacity);
      return;
    }
  }

  if (execArray.Array != nullptr)
  {
    const vtkm::UInt64 cap = static_cast<vtkm::UInt64>(static_cast<char*>(execArray.ArrayCapacity) -
                                                       static_cast<char*>(execArray.Array));

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
    // Cast to char* so that the pointer math below will work.
    char* tmp = static_cast<char*>(CudaAllocator::Allocate(static_cast<size_t>(numBytes)));
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

  // If we just allocated managed cuda memory and don't a host memory pointer
  // we can share out managed memory. This allows for the use case of where we
  // first allocate on CUDA and than want to use it on the host
  if (CudaAllocator::IsManagedPointer(execArray.Array) && execArray.ArrayControl == nullptr)
  {
    this->ControlStorage.SetBasePointer(
      execArray.Array, numberOfValues, sizeOfValue, [](void* ptr) { CudaAllocator::Free(ptr); });
  }
}

void ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::Free(
  TypelessExecutionArray& execArray) const
{
  // If we're sharing a device-accessible pointer between control/exec, don't
  // actually free it -- just null the pointers here:
  if (execArray.Array == execArray.ArrayControl &&
      CudaAllocator::IsDevicePointer(execArray.ArrayControl))
  {
    execArray.Array = nullptr;
    execArray.ArrayEnd = nullptr;
    execArray.ArrayCapacity = nullptr;
    return;
  }

  if (execArray.Array != nullptr)
  {
    CudaAllocator::Free(execArray.Array);
    execArray.Array = nullptr;
    execArray.ArrayEnd = nullptr;
    execArray.ArrayCapacity = nullptr;
  }
}

void ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::CopyFromControl(
  const void* controlPtr,
  void* executionPtr,
  vtkm::UInt64 numBytes) const
{
  // Do nothing if we're sharing a device-accessible pointer between control and
  // execution:
  if (controlPtr == executionPtr && CudaAllocator::IsDevicePointer(controlPtr))
  {
    CudaAllocator::PrepareForInput(executionPtr, numBytes);
    return;
  }

  VTKM_CUDA_CALL(cudaMemcpyAsync(executionPtr,
                                 controlPtr,
                                 static_cast<std::size_t>(numBytes),
                                 cudaMemcpyHostToDevice,
                                 cudaStreamPerThread));
}

void ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::CopyToControl(const void* executionPtr,
                                                                       void* controlPtr,
                                                                       vtkm::UInt64 numBytes) const
{
  // Do nothing if we're sharing a cuda managed pointer between control and execution:
  if (controlPtr == executionPtr && CudaAllocator::IsDevicePointer(controlPtr))
  {
    // If we're trying to copy a shared, non-managed device pointer back to
    // control throw an exception -- the pointer cannot be read from control,
    // so this operation is invalid.
    if (!CudaAllocator::IsManagedPointer(controlPtr))
    {
      throw vtkm::cont::ErrorBadValue(
        "Control pointer is a CUDA device pointer that does not supported managed access.");
    }

    // If it is managed, just return and let CUDA handle the migration for us.
    CudaAllocator::PrepareForControl(controlPtr, numBytes);
  }
  else
  {
    VTKM_CUDA_CALL(cudaMemcpyAsync(controlPtr,
                                   executionPtr,
                                   static_cast<std::size_t>(numBytes),
                                   cudaMemcpyDeviceToHost,
                                   cudaStreamPerThread));
  }

  //In all cases we have possibly multiple async calls queued up in
  //our stream. We need to block on the copy back to control since
  //we don't wanting it accessing memory that hasn't finished
  //being used by the GPU
  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTagCuda>::Synchronize();
}

void ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::UsingForRead(
  const void* vtkmNotUsed(controlPtr),
  const void* executionPtr,
  vtkm::UInt64 numBytes) const
{
  CudaAllocator::PrepareForInput(executionPtr, static_cast<size_t>(numBytes));
}

void ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::UsingForWrite(
  const void* vtkmNotUsed(controlPtr),
  const void* executionPtr,
  vtkm::UInt64 numBytes) const
{
  CudaAllocator::PrepareForOutput(executionPtr, static_cast<size_t>(numBytes));
}

void ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>::UsingForReadWrite(
  const void* vtkmNotUsed(controlPtr),
  const void* executionPtr,
  vtkm::UInt64 numBytes) const
{
  CudaAllocator::PrepareForInPlace(executionPtr, static_cast<size_t>(numBytes));
}


} // end namespace internal
}
} // end vtkm::cont
