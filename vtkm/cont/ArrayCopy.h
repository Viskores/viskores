//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayCopy_h
#define vtk_m_cont_ArrayCopy_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/Logging.h>

// TODO: When virtual arrays are available, compile the implementation in a .cxx/.cu file. Common
// arrays are copied directly but anything else would be copied through virtual methods.

namespace vtkm
{
namespace cont
{
/// \brief Does a deep copy from one array to another array.
///
/// Given a source \c ArrayHandle and a destination \c ArrayHandle, this function allocates the
/// destination \c ArrayHandle to the correct size and deeply copies all the values from the source
/// to the destination.
///
/// This method will attempt to copy the data using the device that the input
/// data is already valid on. If the input data is only valid in the control
/// environment, the runtime device tracker is used to try to find another
/// device.
///
template <typename InValueType, typename InStorage, typename OutValueType, typename OutStorage>
VTKM_CONT inline void ArrayCopy(const vtkm::cont::ArrayHandle<InValueType, InStorage>& source,
                                vtkm::cont::ArrayHandle<OutValueType, OutStorage>& destination)
{
  // Find the device that already has a copy of the data:
  vtkm::cont::DeviceAdapterId devId = source.GetDeviceAdapterId();

  // If the data is not on any device, let the runtime tracker pick an available
  // parallel copy algorithm.
  if (devId.GetValue() == VTKM_DEVICE_ADAPTER_UNDEFINED)
  {
    devId = vtkm::cont::make_DeviceAdapterId(VTKM_DEVICE_ADAPTER_ANY);
  }

  bool success = vtkm::cont::Algorithm::Copy(devId, source, destination);

  if (!success && devId.GetValue() != VTKM_DEVICE_ADAPTER_ANY)
  { // Retry on any device if the first attempt failed.
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "Failed to run ArrayCopy on device '" << devId.GetName()
                                                     << "'. Retrying on any device.");
    success = vtkm::cont::Algorithm::Copy(vtkm::cont::DeviceAdapterTagAny{}, source, destination);
  }

  if (!success)
  {
    throw vtkm::cont::ErrorExecution("Failed to run ArrayCopy on any device.");
  }
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayCopy_h
