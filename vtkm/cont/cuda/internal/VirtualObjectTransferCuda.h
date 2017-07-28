//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_VirtualObjectTransferCuda_h
#define vtk_m_cont_cuda_internal_VirtualObjectTransferCuda_h

#include <vtkm/cont/cuda/ErrorCuda.h>
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

template <typename VirtualObject, typename TargetClass>
__global__ void CreateKernel(VirtualObject* object, const TargetClass* target)
{
  object->Bind(target);
}

} // detail

template <typename VirtualObject, typename TargetClass>
struct VirtualObjectTransfer<VirtualObject, TargetClass, vtkm::cont::DeviceAdapterTagCuda>
{
  static void* Create(VirtualObject& object, const void* target)
  {
    TargetClass* cutarget;
    VTKM_CUDA_CALL(cudaMalloc(&cutarget, sizeof(TargetClass)));
    VTKM_CUDA_CALL(cudaMemcpy(cutarget, target, sizeof(TargetClass), cudaMemcpyHostToDevice));

    VirtualObject* cuobject;
    VTKM_CUDA_CALL(cudaMalloc(&cuobject, sizeof(VirtualObject)));
    detail::CreateKernel<<<1, 1>>>(cuobject, cutarget);
    VTKM_CUDA_CHECK_ASYNCHRONOUS_ERROR();
    VTKM_CUDA_CALL(cudaMemcpy(&object, cuobject, sizeof(VirtualObject), cudaMemcpyDeviceToHost));
    VTKM_CUDA_CALL(cudaFree(cuobject));

    return cutarget;
  }

  static void Update(void* deviceState, const void* target)
  {
    VTKM_CUDA_CALL(cudaMemcpy(deviceState, target, sizeof(TargetClass), cudaMemcpyHostToDevice));
  }

  static void Cleanup(void* deviceState) { VTKM_CUDA_CALL(cudaFree(deviceState)); }
};
}
}
} // vtkm::cont::internal

#endif // vtk_m_cont_cuda_internal_VirtualObjectTransferCuda_h
