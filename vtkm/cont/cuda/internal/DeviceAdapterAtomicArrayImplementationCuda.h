//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_internal_DeviceAdapterAtomicArrayImplementationCuda_h
#define vtk_m_cont_internal_DeviceAdapterAtomicArrayImplementationCuda_h

#include <vtkm/Types.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/StorageBasic.h>

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>

// Disable warnings we check vtkm for but Thrust does not.
#include <vtkm/exec/cuda/internal/ThrustPatches.h>
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/device_ptr.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace cont
{

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

  VTKM_EXEC T Add(vtkm::Id index, const T& value) const
  {
// Although this function is marked VTKM_EXEC, this currently expands to
// __host__ __device__, and nvcc 8.0.61 errors when calling the __device__
// function vtkmAtomicAdd. VTKM_SUPPRESS_EXEC_WARNINGS does not fix this.
// We work around this by calling the __device__ function inside of a
// __CUDA_ARCH__ guard, as nvcc is smart enough to recognize that this is a
// safe usage of a __device__ function in a __host__ __device__ context.
#ifdef VTKM_CUDA_DEVICE_PASS
    T* lockedValue = ::thrust::raw_pointer_cast(this->Portal.GetIteratorBegin() + index);
    return this->vtkmAtomicAdd(lockedValue, value);
#else
    // Shut up, compiler
    (void)index;
    (void)value;

    throw vtkm::cont::ErrorExecution("AtomicArray used in control environment, "
                                     "or incorrect array implementation used "
                                     "for device.");
#endif
  }

  VTKM_EXEC T CompareAndSwap(vtkm::Id index,
                             const vtkm::Int64& newValue,
                             const vtkm::Int64& oldValue) const
  {
// Although this function is marked VTKM_EXEC, this currently expands to
// __host__ __device__, and nvcc 8.0.61 errors when calling the __device__
// function vtkmAtomicAdd. VTKM_SUPPRESS_EXEC_WARNINGS does not fix this.
// We work around this by calling the __device__ function inside of a
// __CUDA_ARCH__ guard, as nvcc is smart enough to recognize that this is a
// safe usage of a __device__ function in a __host__ __device__ context.
#ifdef VTKM_CUDA_DEVICE_PASS
    T* lockedValue = ::thrust::raw_pointer_cast(this->Portal.GetIteratorBegin() + index);
    return this->vtkmCompareAndSwap(lockedValue, newValue, oldValue);
#else
    // Shut up, compiler.
    (void)index;
    (void)newValue;
    (void)oldValue;

    throw vtkm::cont::ErrorExecution("AtomicArray used in control environment, "
                                     "or incorrect array implementation used "
                                     "for device.");
#endif
  }

private:
  using PortalType =
    typename vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>::template ExecutionTypes<
      vtkm::cont::DeviceAdapterTagCuda>::Portal;
  PortalType Portal;

  VTKM_SUPPRESS_EXEC_WARNINGS
  __device__ vtkm::Int64 vtkmAtomicAdd(vtkm::Int64* address, const vtkm::Int64& value) const
  {
    return atomicAdd((unsigned long long*)address, (unsigned long long)value);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  __device__ vtkm::Int32 vtkmAtomicAdd(vtkm::Int32* address, const vtkm::Int32& value) const
  {
    return atomicAdd(address, value);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  __device__ vtkm::Int32 vtkmCompareAndSwap(vtkm::Int32* address,
                                            const vtkm::Int32& newValue,
                                            const vtkm::Int32& oldValue) const
  {
    return atomicCAS(address, oldValue, newValue);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  __device__ vtkm::Int64 vtkmCompareAndSwap(vtkm::Int64* address,
                                            const vtkm::Int64& newValue,
                                            const vtkm::Int64& oldValue) const
  {
    return atomicCAS((unsigned long long int*)address,
                     (unsigned long long int)oldValue,
                     (unsigned long long int)newValue);
  }
};
}
} // end namespace vtkm::cont

#endif // vtk_m_cont_internal_DeviceAdapterAtomicArrayImplementationCuda_h
