//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_tbb_internal_RuntimeDeviceConfigurationTBB_h
#define vtk_m_cont_tbb_internal_RuntimeDeviceConfigurationTBB_h

#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
class RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagTBB>
  : public vtkm::cont::internal::RuntimeDeviceConfigurationBase
{
  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const override final
  {
    return vtkm::cont::DeviceAdapterTagTBB{};
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetThreads(const vtkm::Id&) override final
  {
    // TODO: vtk-m set the number of global threads
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetThreads(vtkm::Id&) const override final
  {
    // TODO: Get number of TBB threads here (essentially just threads supported by architecture)
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetMaxThreads(vtkm::Id&) const override final
  {
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }
};
} // namespace vktm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif //vtk_m_cont_tbb_internal_RuntimeDeviceConfigurationTBB_h
