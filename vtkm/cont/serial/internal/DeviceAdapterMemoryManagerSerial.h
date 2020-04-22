//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_serial_internal_DeviceAdapterMemoryManagerSerial_h
#define vtk_m_cont_serial_internal_DeviceAdapterMemoryManagerSerial_h

#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

#include <vtkm/cont/internal/DeviceAdapterMemoryManagerShared.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
class DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagSerial>
  : public vtkm::cont::internal::DeviceAdapterMemoryManagerShared
{
  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const override
  {
    return vtkm::cont::DeviceAdapterTagSerial{};
  }
};
}
}
}

#endif //vtk_m_cont_serial_internal_DeviceAdapterMemoryManagerSerial_h
