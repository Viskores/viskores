//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/internal/RuntimeDeviceConfigurationOptions.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

RuntimeDeviceConfigurationOptions::RuntimeDeviceConfigurationOptions()
  : VTKmNumThreads(option::OptionIndex::NUM_THREADS, "VTKM_NUM_THREADS")
  , VTKmNumaRegions(option::OptionIndex::NUMA_REGIONS, "VTKM_NUMA_REGIONS")
  , VTKmDeviceInstance(option::OptionIndex::DEVICE_INSTANCE, "VTKM_DEVICE_INSTANCE")
  , Initialized(false)
{
}

RuntimeDeviceConfigurationOptions::~RuntimeDeviceConfigurationOptions() noexcept = default;

RuntimeDeviceConfigurationOptions::RuntimeDeviceConfigurationOptions(
  std::vector<option::Descriptor>& usage)
  : RuntimeDeviceConfigurationOptions()
{
  usage.push_back(
    { option::OptionIndex::NUM_THREADS,
      0,
      "",
      "vtkm-num-threads",
      option::VtkmArg::Required,
      "  --vtkm-num-threads <dev> \tSets the number of threads to use for the selected device" });
  usage.push_back(
    { option::OptionIndex::NUMA_REGIONS,
      0,
      "",
      "vtkm-numa-regions",
      option::VtkmArg::Required,
      "  --vtkm-numa-regions <dev> \tSets the number of numa regions when using kokkos/OpenMP" });
  usage.push_back({ option::OptionIndex::DEVICE_INSTANCE,
                    0,
                    "",
                    "vtkm-device-instance",
                    option::VtkmArg::Required,
                    "  --vtkm-device-instance <dev> \tSets the device instance to use when using "
                    "kokkos/cuda" });
}

void RuntimeDeviceConfigurationOptions::Initialize(const option::Option* options)
{
  this->VTKmNumThreads.Initialize(options);
  this->VTKmNumaRegions.Initialize(options);
  this->VTKmDeviceInstance.Initialize(options);
  this->Initialized = true;
}

bool RuntimeDeviceConfigurationOptions::IsInitialized() const
{
  return this->Initialized;
}

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm
