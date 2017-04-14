//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_h
#define vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_h

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>

#include <vtkm/cont/Storage.h>
#include <vtkm/cont/internal/ArrayManagerExecution.h>
#include <vtkm/cont/cuda/internal/ArrayManagerExecutionThrustDevice.h>

// These must be placed in the vtkm::cont::internal namespace so that
// the template can be found.

namespace vtkm {
namespace cont {
namespace internal {

template <typename T, class StorageTag>
class ArrayManagerExecution
    <T, StorageTag, vtkm::cont::DeviceAdapterTagCuda>
    : public vtkm::cont::cuda::internal::ArrayManagerExecutionThrustDevice
        <T, StorageTag>
{
public:
  typedef vtkm::cont::cuda::internal::ArrayManagerExecutionThrustDevice
      <T, StorageTag> Superclass;
  typedef typename Superclass::ValueType ValueType;
  typedef typename Superclass::PortalType PortalType;
  typedef typename Superclass::PortalConstType PortalConstType;
  typedef typename Superclass::StorageType StorageType;

  VTKM_CONT
  ArrayManagerExecution(StorageType *storage)
    : Superclass(storage) {  }

  VTKM_CONT
  PortalConstType PrepareForInput(bool updateData)
  {
    try
    {
      // This alternate form of PrepareForInput works around an issue
      // with nvcc 7.5.
      return this->Superclass::template _PrepareForInput<void>(updateData);
    }
    catch (vtkm::cont::ErrorBadAllocation &error)
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
    catch (vtkm::cont::ErrorBadAllocation &error)
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
    catch (vtkm::cont::ErrorBadAllocation &error)
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

}
}
} // namespace vtkm::cont::internal


#endif //vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_h
