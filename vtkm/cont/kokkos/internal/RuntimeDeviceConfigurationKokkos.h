//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_kokkos_internal_RuntimeDeviceConfigurationKokkos_h
#define vtk_m_cont_kokkos_internal_RuntimeDeviceConfigurationKokkos_h

#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>
#include <vtkm/cont/kokkos/internal/DeviceAdapterTagKokkos.h>

#include <Kokkos_Core.hpp>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
class RuntimeDeviceConfiguration<vtkm::cont::DeviceAdapterTagKokkos>
  : public vtkm::cont::internal::RuntimeDeviceConfigurationBase
{
public:
  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const override final
  {
    return vtkm::cont::DeviceAdapterTagKokkos{};
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetThreads(const vtkm::Id&) const override final
  {
    // TODO: set the kokkos threads
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetNumaRegions(
    const vtkm::Id&) const override final
  {
    // TODO: set the kokkos numa regions
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetDeviceInstance(
    const vtkm::Id&) const override final
  {
    // TODO: set the kokkos device instance
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetThreads(vtkm::Id&) const override final
  {
    // TODO: get the kokkos threads
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetNumaRegions(vtkm::Id&) const override final
  {
    // TODO: get the kokkos numa regions
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetDeviceInstance(vtkm::Id&) const override final
  {
    // TODO: get the kokkos device instance
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

protected:
  VTKM_CONT virtual void ParseExtraArguments(int&, char*[]) const override final
  {
    // TODO: ugh, kokkos. Manually parse the kokkos config args, store them for usage
  }

private:
  Kokkos::InitArguments ParsedCommandLineArgs;
  Kokkos::InitArguments VTKmInitializedArgs;
};
} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif //vtk_m_cont_kokkos_internal_RuntimeDeviceConfigurationKokkos_h
