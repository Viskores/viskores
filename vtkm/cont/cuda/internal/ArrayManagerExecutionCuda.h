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
#ifndef vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_h
#define vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_h

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>

#include <vtkm/cont/cuda/internal/ArrayManagerExecutionThrustDevice.h>
#include <vtkm/cont/internal/ArrayExportMacros.h>
#include <vtkm/cont/internal/ArrayManagerExecution.h>

// These must be placed in the vtkm::cont::internal namespace so that
// the template can be found.

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename T, class StorageTag>
class ArrayManagerExecution<T, StorageTag, vtkm::cont::DeviceAdapterTagCuda>
  : public vtkm::cont::cuda::internal::ArrayManagerExecutionThrustDevice<T, StorageTag>
{
public:
  using Superclass = vtkm::cont::cuda::internal::ArrayManagerExecutionThrustDevice<T, StorageTag>;
  using ValueType = typename Superclass::ValueType;
  using PortalType = typename Superclass::PortalType;
  using PortalConstType = typename Superclass::PortalConstType;
  using StorageType = typename Superclass::StorageType;

  VTKM_CONT
  ArrayManagerExecution(StorageType* storage)
    : Superclass(storage)
  {
  }

  VTKM_CONT
  PortalConstType PrepareForInput(bool updateData)
  {
    try
    {
      // This alternate form of PrepareForInput works around an issue
      // with nvcc 7.5.
      return this->Superclass::template _PrepareForInput<void>(updateData);
    }
    catch (vtkm::cont::ErrorBadAllocation& error)
    {
      // Thrust does not seem to be clearing the CUDA error, so do it here.
      cudaError_t cudaError = cudaPeekAtLastError();
      if (cudaError == cudaErrorMemoryAllocation)
      {
        cudaGetLastError();
      }
      throw error;
    }
  }

  VTKM_CONT
  PortalType PrepareForInPlace(bool updateData)
  {
    try
    {
      // This alternate form of PrepareForInPlace works around an issue
      // with nvcc 7.5.
      return this->Superclass::template _PrepareForInPlace<void>(updateData);
    }
    catch (vtkm::cont::ErrorBadAllocation& error)
    {
      // Thrust does not seem to be clearing the CUDA error, so do it here.
      cudaError_t cudaError = cudaPeekAtLastError();
      if (cudaError == cudaErrorMemoryAllocation)
      {
        cudaGetLastError();
      }
      throw error;
    }
  }

  VTKM_CONT
  PortalType PrepareForOutput(vtkm::Id numberOfValues)
  {
    try
    {
      // This alternate form of PrepareForOutput works around an issue
      // with nvcc 7.5.
      return this->Superclass::template _PrepareForOutput<void>(numberOfValues);
    }
    catch (vtkm::cont::ErrorBadAllocation& error)
    {
      // Thrust does not seem to be clearing the CUDA error, so do it here.
      cudaError_t cudaError = cudaPeekAtLastError();
      if (cudaError == cudaErrorMemoryAllocation)
      {
        cudaGetLastError();
      }
      throw error;
    }
  }
};

template <typename T>
struct ExecutionPortalFactoryBasic<T, DeviceAdapterTagCuda>
{
  using ValueType = T;
  using PortalType = vtkm::exec::cuda::internal::ArrayPortalFromThrust<ValueType>;
  using PortalConstType = vtkm::exec::cuda::internal::ConstArrayPortalFromThrust<ValueType>;

  VTKM_CONT
  static PortalType CreatePortal(ValueType* start, ValueType* end)
  {
    using ThrustPointerT = thrust::system::cuda::pointer<ValueType>;
    ThrustPointerT startThrust(start);
    ThrustPointerT endThrust(end);
    return PortalType(startThrust, endThrust);
  }

  VTKM_CONT
  static PortalConstType CreatePortalConst(const ValueType* start, const ValueType* end)
  {
    using ThrustPointerT = thrust::system::cuda::pointer<const ValueType>;
    ThrustPointerT startThrust(start);
    ThrustPointerT endThrust(end);
    return PortalConstType(startThrust, endThrust);
  }
};

template <>
struct VTKM_CONT_EXPORT ExecutionArrayInterfaceBasic<DeviceAdapterTagCuda>
  : public ExecutionArrayInterfaceBasicBase
{
  using Superclass = ExecutionArrayInterfaceBasicBase;

  VTKM_CONT ExecutionArrayInterfaceBasic(StorageBasicBase& storage);
  VTKM_CONT DeviceAdapterId GetDeviceId() const final;
  VTKM_CONT void Allocate(TypelessExecutionArray& execArray, vtkm::UInt64 numBytes) const final;
  VTKM_CONT void Free(TypelessExecutionArray& execArray) const final;
  VTKM_CONT void CopyFromControl(const void* controlPtr,
                                 void* executionPtr,
                                 vtkm::UInt64 numBytes) const final;
  VTKM_CONT void CopyToControl(const void* executionPtr,
                               void* controlPtr,
                               vtkm::UInt64 numBytes) const final;

  VTKM_CONT void UsingForRead(const void* controlPtr,
                              const void* executionPtr,
                              vtkm::UInt64 numBytes) const final;
  VTKM_CONT void UsingForWrite(const void* controlPtr,
                               const void* executionPtr,
                               vtkm::UInt64 numBytes) const final;
  VTKM_CONT void UsingForReadWrite(const void* controlPtr,
                                   const void* executionPtr,
                                   vtkm::UInt64 numBytes) const final;
};
} // namespace internal

#ifndef vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_cu
VTKM_EXPORT_ARRAYHANDLES_FOR_DEVICE_ADAPTER(DeviceAdapterTagCuda)
#endif // !vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_cu
}
} // namespace vtkm::cont

#endif //vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_h
