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
