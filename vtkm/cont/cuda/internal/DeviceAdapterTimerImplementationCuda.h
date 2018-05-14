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
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterTimerImplementationCuda_h
#define vtk_m_cont_cuda_internal_DeviceAdapterTimerImplementationCuda_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>

#include <cuda.h>

namespace vtkm
{
namespace cont
{

///
/// Specialization of DeviceAdapterTimerImplementation for CUDA
/// CUDA contains its own high resolution timer that are able
/// to track how long it takes to execute async kernels.
/// If we simply measured time on the CPU it would incorrectly
/// just capture how long it takes to launch a kernel.
template <>
class VTKM_CONT_EXPORT DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>
{
public:
  VTKM_CONT DeviceAdapterTimerImplementation();

  VTKM_CONT ~DeviceAdapterTimerImplementation();

  VTKM_CONT void Reset();

  VTKM_CONT vtkm::Float64 GetElapsedTime();

private:
  // Copying CUDA events is problematic.
  DeviceAdapterTimerImplementation(
    const DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>&) = delete;
  void operator=(const DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>&) =
    delete;

  cudaEvent_t StartEvent;
  cudaEvent_t EndEvent;
};
}
} // namespace vtkm::cont


#endif
