//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_VirtualObjectTransferCuda_h
#define vtk_m_cont_cuda_internal_VirtualObjectTransferCuda_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/cuda/ErrorCuda.h>
#include <vtkm/cont/cuda/internal/CudaAllocator.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/internal/VirtualObjectTransfer.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

namespace detail
{

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

template <typename VirtualDerivedType>
__global__ void ConstructVirtualObjectKernel(VirtualDerivedType* deviceObject,
                                             const VirtualDerivedType* targetObject)
{
  // Use the "placement new" syntax to construct an object in pre-allocated memory
  new (deviceObject) VirtualDerivedType(*targetObject);
}

template <typename VirtualDerivedType>
__global__ void UpdateVirtualObjectKernel(VirtualDerivedType* deviceObject,
                                          const VirtualDerivedType* targetObject)
{
  *deviceObject = *targetObject;
}

template <typename VirtualDerivedType>
__global__ void DeleteVirtualObjectKernel(VirtualDerivedType* deviceObject)
{
  deviceObject->~VirtualDerivedType();
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif

} // detail

template <typename VirtualDerivedType>
struct VirtualObjectTransfer<VirtualDerivedType, vtkm::cont::DeviceAdapterTagCuda>
{
  VTKM_CONT VirtualObjectTransfer(const VirtualDerivedType* virtualObject)
    : ControlObject(virtualObject)
    , ExecutionObject(nullptr)
  {
  }

  VTKM_CONT ~VirtualObjectTransfer() { this->ReleaseResources(); }

  VirtualObjectTransfer(const VirtualObjectTransfer&) = delete;
  void operator=(const VirtualObjectTransfer&) = delete;

  VTKM_CONT const VirtualDerivedType* PrepareForExecution(bool updateData)
  {
    if (this->ExecutionObject == nullptr)
    {
      // deviceTarget will hold a byte copy of the host object on the device. The virtual table
      // will be wrong.
      VirtualDerivedType* deviceTarget;
      VTKM_CUDA_CALL(cudaMalloc(&deviceTarget, sizeof(VirtualDerivedType)));
      VTKM_CUDA_CALL(cudaMemcpyAsync(deviceTarget,
                                     this->ControlObject,
                                     sizeof(VirtualDerivedType),
                                     cudaMemcpyHostToDevice,
                                     cudaStreamPerThread));

      // Allocate memory for the object that will eventually be a correct copy on the device.
      VTKM_CUDA_CALL(cudaMalloc(&this->ExecutionObject, sizeof(VirtualDerivedType)));

      // Initialize the device object
      detail::ConstructVirtualObjectKernel<<<1, 1, 0, cudaStreamPerThread>>>(this->ExecutionObject,
                                                                             deviceTarget);
      VTKM_CUDA_CHECK_ASYNCHRONOUS_ERROR();

      // Clean up intermediate copy
      vtkm::cont::cuda::internal::CudaAllocator::FreeDeferred(deviceTarget,
                                                              sizeof(VirtualDerivedType));
    }
    else if (updateData)
    {
      // deviceTarget will hold a byte copy of the host object on the device. The virtual table
      // will be wrong.
      VirtualDerivedType* deviceTarget;
      VTKM_CUDA_CALL(cudaMalloc(&deviceTarget, sizeof(VirtualDerivedType)));
      VTKM_CUDA_CALL(cudaMemcpyAsync(deviceTarget,
                                     this->ControlObject,
                                     sizeof(VirtualDerivedType),
                                     cudaMemcpyHostToDevice,
                                     cudaStreamPerThread));

      // Initialize the device object
      detail::UpdateVirtualObjectKernel<<<1, 1, 0, cudaStreamPerThread>>>(this->ExecutionObject,
                                                                          deviceTarget);
      VTKM_CUDA_CHECK_ASYNCHRONOUS_ERROR();

      // Clean up intermediate copy
      vtkm::cont::cuda::internal::CudaAllocator::FreeDeferred(deviceTarget,
                                                              sizeof(VirtualDerivedType));
    }
    else
    {
      // Nothing to do. The device object is already up to date.
    }

    return this->ExecutionObject;
  }

  VTKM_CONT void ReleaseResources()
  {
    if (this->ExecutionObject != nullptr)
    {
      detail::DeleteVirtualObjectKernel<<<1, 1, 0, cudaStreamPerThread>>>(this->ExecutionObject);
      vtkm::cont::cuda::internal::CudaAllocator::FreeDeferred(this->ExecutionObject,
                                                              sizeof(VirtualDerivedType));
      this->ExecutionObject = nullptr;
    }
  }

private:
  const VirtualDerivedType* ControlObject;
  VirtualDerivedType* ExecutionObject;
};
}
}
} // vtkm::cont::internal

#define VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(DerivedType)                                          \
  template class vtkm::cont::internal::VirtualObjectTransfer<DerivedType,                          \
                                                             vtkm::cont::DeviceAdapterTagCuda>

#endif // vtk_m_cont_cuda_internal_VirtualObjectTransferCuda_h
