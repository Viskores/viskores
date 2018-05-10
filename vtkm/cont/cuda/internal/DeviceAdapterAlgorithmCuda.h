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
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h
#define vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h

#include <vtkm/cont/cuda/ErrorCuda.h>
#include <vtkm/cont/cuda/internal/ArrayManagerExecutionCuda.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/Math.h>

// Here are the actual implementation of the algorithms.
#include <vtkm/cont/cuda/internal/DeviceAdapterAlgorithmThrust.h>

// Here are the implementations of device adapter specific classes
#include <vtkm/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTimerImplementationCuda.h>

#include <vtkm/exec/cuda/internal/TaskStrided.h>

#include <cuda.h>

namespace vtkm
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>
  : public vtkm::cont::cuda::internal::DeviceAdapterAlgorithmThrust<
      vtkm::cont::DeviceAdapterTagCuda>
{

  VTKM_CONT static void Synchronize()
  {
    VTKM_CUDA_CALL(cudaStreamSynchronize(cudaStreamPerThread));
  }
};

/// CUDA contains its own atomic operations
///
template <typename T>
class DeviceAdapterAtomicArrayImplementation<T, vtkm::cont::DeviceAdapterTagCuda>
{
public:
  VTKM_CONT
  DeviceAdapterAtomicArrayImplementation(
    vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic> handle)
    : Portal(handle.PrepareForInPlace(vtkm::cont::DeviceAdapterTagCuda()))
  {
  }

  inline __device__ T Add(vtkm::Id index, const T& value) const
  {
    T* lockedValue = ::thrust::raw_pointer_cast(this->Portal.GetIteratorBegin() + index);
    return vtkmAtomicAdd(lockedValue, value);
  }

  inline __device__ T CompareAndSwap(vtkm::Id index,
                                     const vtkm::Int64& newValue,
                                     const vtkm::Int64& oldValue) const
  {
    T* lockedValue = ::thrust::raw_pointer_cast(this->Portal.GetIteratorBegin() + index);
    return vtkmCompareAndSwap(lockedValue, newValue, oldValue);
  }

private:
  using PortalType =
    typename vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>::template ExecutionTypes<
      vtkm::cont::DeviceAdapterTagCuda>::Portal;
  PortalType Portal;

  inline __device__ vtkm::Int64 vtkmAtomicAdd(vtkm::Int64* address, const vtkm::Int64& value) const
  {
    return atomicAdd((unsigned long long*)address, (unsigned long long)value);
  }

  inline __device__ vtkm::Int32 vtkmAtomicAdd(vtkm::Int32* address, const vtkm::Int32& value) const
  {
    return atomicAdd(address, value);
  }

  inline __device__ vtkm::Int32 vtkmCompareAndSwap(vtkm::Int32* address,
                                                   const vtkm::Int32& newValue,
                                                   const vtkm::Int32& oldValue) const
  {
    return atomicCAS(address, oldValue, newValue);
  }

  inline __device__ vtkm::Int64 vtkmCompareAndSwap(vtkm::Int64* address,
                                                   const vtkm::Int64& newValue,
                                                   const vtkm::Int64& oldValue) const
  {
    return atomicCAS((unsigned long long int*)address,
                     (unsigned long long int)oldValue,
                     (unsigned long long int)newValue);
  }
};

template <>
class DeviceTaskTypes<vtkm::cont::DeviceAdapterTagCuda>
{
public:
  template <typename WorkletType, typename InvocationType>
  static vtkm::exec::cuda::internal::TaskStrided1D<WorkletType, InvocationType> MakeTask(
    WorkletType& worklet,
    InvocationType& invocation,
    vtkm::Id,
    vtkm::Id globalIndexOffset = 0)
  {
    using Task = vtkm::exec::cuda::internal::TaskStrided1D<WorkletType, InvocationType>;
    return Task(worklet, invocation, globalIndexOffset);
  }

  template <typename WorkletType, typename InvocationType>
  static vtkm::exec::cuda::internal::TaskStrided3D<WorkletType, InvocationType> MakeTask(
    WorkletType& worklet,
    InvocationType& invocation,
    vtkm::Id3,
    vtkm::Id globalIndexOffset = 0)
  {
    using Task = vtkm::exec::cuda::internal::TaskStrided3D<WorkletType, InvocationType>;
    return Task(worklet, invocation, globalIndexOffset);
  }
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h
