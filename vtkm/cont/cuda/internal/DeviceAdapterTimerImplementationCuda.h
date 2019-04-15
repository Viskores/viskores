//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

  VTKM_CONT void Start();

  VTKM_CONT void Stop();

  VTKM_CONT bool Started() const;

  VTKM_CONT bool Stopped() const;

  VTKM_CONT bool Ready() const;

  VTKM_CONT vtkm::Float64 GetElapsedTime() const;

private:
  // Copying CUDA events is problematic.
  DeviceAdapterTimerImplementation(
    const DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>&) = delete;
  void operator=(const DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>&) =
    delete;

  bool StartReady;
  bool StopReady;
  cudaEvent_t StartEvent;
  cudaEvent_t StopEvent;
};
}
} // namespace vtkm::cont


#endif
