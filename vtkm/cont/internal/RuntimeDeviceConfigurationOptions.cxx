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

#include <memory>
#include <sstream>

namespace vtkm
{
namespace cont
{
namespace internal
{

namespace
{
void AppendOptionDescriptors(std::vector<option::Descriptor>& usage,
                             const bool& useOptionIndex = true)
{
  usage.push_back(
    { useOptionIndex ? static_cast<uint32_t>(option::OptionIndex::NUM_THREADS) : 0,
      0,
      "",
      "vtkm-num-threads",
      option::VtkmArg::Required,
      "  --vtkm-num-threads <dev> \tSets the number of threads to use for the selected device" });
  usage.push_back(
    { useOptionIndex ? static_cast<uint32_t>(option::OptionIndex::NUMA_REGIONS) : 1,
      0,
      "",
      "vtkm-numa-regions",
      option::VtkmArg::Required,
      "  --vtkm-numa-regions <dev> \tSets the number of numa regions when using kokkos/OpenMP" });
  usage.push_back(
    { useOptionIndex ? static_cast<uint32_t>(option::OptionIndex::DEVICE_INSTANCE) : 2,
      0,
      "",
      "vtkm-device-instance",
      option::VtkmArg::Required,
      "  --vtkm-device-instance <dev> \tSets the device instance to use when using "
      "kokkos/cuda" });
}
} // anonymous namespace

RuntimeDeviceConfigurationOptions::RuntimeDeviceConfigurationOptions(const bool& useOptionIndex)
  : VTKmNumThreads(useOptionIndex ? option::OptionIndex::NUM_THREADS : 0, "VTKM_NUM_THREADS")
  , VTKmNumaRegions(useOptionIndex ? option::OptionIndex::NUMA_REGIONS : 1, "VTKM_NUMA_REGIONS")
  , VTKmDeviceInstance(useOptionIndex ? option::OptionIndex::DEVICE_INSTANCE : 2,
                       "VTKM_DEVICE_INSTANCE")
  , Initialized(false)
{
}

RuntimeDeviceConfigurationOptions::RuntimeDeviceConfigurationOptions()
  : RuntimeDeviceConfigurationOptions(true)
{
}


RuntimeDeviceConfigurationOptions::RuntimeDeviceConfigurationOptions(
  std::vector<option::Descriptor>& usage)
  : RuntimeDeviceConfigurationOptions(true)
{
  AppendOptionDescriptors(usage);
}

RuntimeDeviceConfigurationOptions::RuntimeDeviceConfigurationOptions(int& argc, char* argv[])
  : RuntimeDeviceConfigurationOptions(false)
{
  std::vector<option::Descriptor> usage;
  AppendOptionDescriptors(usage, false);
  usage.push_back({ option::OptionIndex::UNKNOWN, 0, "", "", option::VtkmArg::UnknownOption, "" });
  usage.push_back({ 0, 0, 0, 0, 0, 0 });

  option::Stats stats(usage.data(), argc, argv);
  std::unique_ptr<option::Option[]> options{ new option::Option[stats.options_max] };
  std::unique_ptr<option::Option[]> buffer{ new option::Option[stats.buffer_max] };
  option::Parser parse(usage.data(), argc, argv, options.get(), buffer.get());

  if (parse.error())
  {
    std::stringstream streamBuffer;
    option::printUsage(streamBuffer, usage.data());
    std::cerr << streamBuffer.str();
    exit(1);
  }

  this->Initialize(options.get());
}

RuntimeDeviceConfigurationOptions::~RuntimeDeviceConfigurationOptions() noexcept = default;

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
