//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_openmp_internal_ExecutionArrayInterfaceBasicOpenMP_h
#define vtk_m_cont_openmp_internal_ExecutionArrayInterfaceBasicOpenMP_h

#include <vtkm/cont/internal/ArrayManagerExecutionShareWithControl.h>
#include <vtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
struct VTKM_CONT_EXPORT ExecutionArrayInterfaceBasic<DeviceAdapterTagOpenMP> final
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
