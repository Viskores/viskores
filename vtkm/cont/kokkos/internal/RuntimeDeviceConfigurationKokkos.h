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

  VTKM_CONT void Initialize(const RuntimeDeviceConfigurationOptions&) const override final
  {
    // TODO: load the kokkos config options
  }

  VTKM_CONT void Initialize(const RuntimeDeviceConfigurationOptions& configOptions,
                            int& argc,
                            char* argv[]) const override final
  {
    // TODO: load the --kokkos command line args and store them for setting later
    this->ParseKokkosArgs(argc, argv);
    this->Initialize(configOptions);
  }

  VTKM_CONT void SetThreads(const vtkm::Id& numThreads) const override final
  {
    // TODO: set the kokkos config object's num threads
  }

  VTKM_CONT void SetNumaRegions(const vtkm::Id& numaRegions) const override final
  {
    // TODO: set the kokkos config object's numa regions
  }

  VTKM_CONT void SetDeviceInstance(const vtkm::Id& deviceInstance) const override final
  {
    // TODO: set the kokkos config object's device instance
  }

  VTKM_CONT vtkm::Id GetThreads() const override final
  {
    // TODO: get the value of the kokkos config object's num threads
  }

  VTKM_CONT vtkm::Id GetNumaRegions() const override final
  {
    // TODO: get the value of the kokkos config object's numa regions
  }

  VTKM_CONT vtkm::Id GetDeviceInstance() const override final
  {
    // TODO: get the value of the kokkos config object's device instance
  }

  VTKM_CONT void ParseKokkosArgs(int& argc, char* argv[]) const
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
