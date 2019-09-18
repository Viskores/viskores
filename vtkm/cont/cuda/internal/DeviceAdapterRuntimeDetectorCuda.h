//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterRuntimeDetectorCuda_h
#define vtk_m_cont_cuda_internal_DeviceAdapterRuntimeDetectorCuda_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>

namespace vtkm
{
namespace cont
{

template <class DeviceAdapterTag>
class DeviceAdapterRuntimeDetector;


/// \brief Class providing a CUDA runtime support detector.
///
/// The class provide the actual implementation used by
/// vtkm::cont::RuntimeDeviceInformation for the CUDA backend.
///
/// We will verify at runtime that the machine has at least one CUDA
/// capable device, and said device is from the 'fermi' (SM_20) generation
/// or newer.
///
template <>
class VTKM_CONT_EXPORT DeviceAdapterRuntimeDetector<vtkm::cont::DeviceAdapterTagCuda>
{
public:
  VTKM_CONT DeviceAdapterRuntimeDetector();

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  /// Only returns true if we have at-least one CUDA capable device of SM_20 or
  /// greater ( fermi ).
  ///
  VTKM_CONT bool Exists() const;

private:
  vtkm::Int32 NumberOfDevices;
  vtkm::Int32 HighestArchSupported;
};
}
} // namespace vtkm::cont


#endif
