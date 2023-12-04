//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_serial_internal_RuntimeDeviceConfigurationSerial_h
#define vtk_m_cont_serial_internal_RuntimeDeviceConfigurationSerial_h

#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>
#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
class RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagSerial>
  : public vtkm::cont::internal::RuntimeDeviceConfigurationBase
{
  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const final
  {
    return vtkm::cont::DeviceAdapterTagSerial{};
  }
};
}
}
}

#endif //vtk_m_cont_serial_internal_RuntimeDeviceConfigurationSerial_h
