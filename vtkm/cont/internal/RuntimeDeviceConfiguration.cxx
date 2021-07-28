//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

RuntimeDeviceConfigurationBase::~RuntimeDeviceConfigurationBase() noexcept = default;

void RuntimeDeviceConfigurationBase::Initialize(
  const RuntimeDeviceConfigurationOptions& configOptions) const
{
  if (configOptions.VTKmNumThreads.IsSet())
  {
    auto value = configOptions.VTKmNumThreads.GetValue();
    auto code = this->SetThreads(value);
    this->LogReturnCode(code, "SetThreads", value);
  }
  if (configOptions.VTKmNumaRegions.IsSet())
  {
    auto value = configOptions.VTKmNumaRegions.GetValue();
    auto code = this->SetNumaRegions(value);
    this->LogReturnCode(code, "SetNumaRegions", value);
  }
  if (configOptions.VTKmDeviceInstance.IsSet())
  {
    auto value = configOptions.VTKmDeviceInstance.GetValue();
    auto code = this->SetDeviceInstance(value);
    this->LogReturnCode(code, "SetDeviceInstance", value);
  }
}

void RuntimeDeviceConfigurationBase::Initialize(
  const RuntimeDeviceConfigurationOptions& configOptions,
  int& argc,
  char* argv[]) const
{
  this->ParseExtraArguments(argc, argv);
  this->Initialize(configOptions);
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::SetThreads(const vtkm::Id&) const
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::SetNumaRegions(const vtkm::Id&) const
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::SetDeviceInstance(
  const vtkm::Id&) const
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::GetThreads(vtkm::Id&) const
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::GetNumaRegions(vtkm::Id&) const
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::GetDeviceInstance(vtkm::Id&) const
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

void RuntimeDeviceConfigurationBase::ParseExtraArguments(int&, char*[]) const {}

void RuntimeDeviceConfigurationBase::LogReturnCode(const RuntimeDeviceConfigReturnCode& code,
                                                   const std::string& function,
                                                   const vtkm::Id& value) const
{
  // Note that we intentionally are not logging a warning for INVALID_FOR_DEVICE. When a
  // user provides a command line argument, it gets sent to all possible devices during
  // `Initialize` regardless of whether it is used. The user does not need a lot of
  // useless warnings about (for example) the serial device not supporting parameters
  // intended for a real parallel device.
  if (code == RuntimeDeviceConfigReturnCode::OUT_OF_BOUNDS)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               function << " for " << this->GetDevice().GetName()
                        << "was OUT_OF_BOUNDS with value: " << value);
  }
  else if (code == RuntimeDeviceConfigReturnCode::INVALID_VALUE)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               function << "for " << this->GetDevice().GetName()
                        << "had INVLAID_VALUE for value: " << value);
  }
#ifndef VTKM_ENABLE_LOGGING
  (void)function;
  (void)value;
#endif
}

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm
