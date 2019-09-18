//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_serial_internal_ExecutionArrayInterfaceBasicSerial_h
#define vtk_m_cont_serial_internal_ExecutionArrayInterfaceBasicSerial_h

#include <vtkm/cont/internal/ArrayManagerExecutionShareWithControl.h>
#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
struct VTKM_CONT_EXPORT ExecutionArrayInterfaceBasic<DeviceAdapterTagSerial> final
  : public ExecutionArrayInterfaceBasicShareWithControl
{
  //inherit our parents constructor
  using ExecutionArrayInterfaceBasicShareWithControl::ExecutionArrayInterfaceBasicShareWithControl;

  VTKM_CONT
  DeviceAdapterId GetDeviceId() const final;
};

} // namespace internal
}
} // namespace vtkm::cont

#endif //vtk_m_cont_serial_internal_ExecutionArrayInterfaceBasicSerial_h
