//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_serial_internal_DeviceAdapterRuntimeDetector_h
#define vtk_m_cont_serial_internal_DeviceAdapterRuntimeDetector_h

#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>
#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{

template <class DeviceAdapterTag>
class DeviceAdapterRuntimeDetector;

/// Determine if this machine supports Serial backend
///
template <>
class VTKM_CONT_EXPORT DeviceAdapterRuntimeDetector<vtkm::cont::DeviceAdapterTagSerial>
{
public:
  /// Returns true if the given device adapter is supported on the current
  /// machine.
  VTKM_CONT bool Exists() const;
};
}
}

#endif
