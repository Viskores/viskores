//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

namespace
{
VTKM_CONT
std::string RuntimeDeviceConfigReturnCodeToString(const RuntimeDeviceConfigReturnCode& code)
{
  switch (code)
  {
    case RuntimeDeviceConfigReturnCode::SUCCESS:
      return "SUCCESS";
    case RuntimeDeviceConfigReturnCode::OUT_OF_BOUNDS:
      return "OUT_OF_BOUNDS";
    case RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE:
      return "INVALID_FOR_DEVICE";
    case RuntimeDeviceConfigReturnCode::INVALID_VALUE:
      return "INVALID_VALUE";
    case RuntimeDeviceConfigReturnCode::NOT_APPLIED:
      return "NOT_APPLIED";
    default:
      return "";
  }
}

VTKM_CONT
void LogReturnCode(const RuntimeDeviceConfigReturnCode& code,
                   const std::string& function,
                   const vtkm::Id& value,
                   const std::string& deviceName)
{
  // Note that we intentionally are not logging a warning for INVALID_FOR_DEVICE. When a
  // user provides a command line argument, it gets sent to all possible devices during
  // `Initialize` regardless of whether it is used. The user does not need a lot of
  // useless warnings about (for example) the serial device not supporting parameters
  // intended for a real parallel device.
  if ((code != RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE) &&
      (code != RuntimeDeviceConfigReturnCode::SUCCESS))
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               function << " for device: " << deviceName << " had code: "
                        << RuntimeDeviceConfigReturnCodeToString(code) << " with value: " << value);
  }
#ifndef VTKM_ENABLE_LOGGING
  (void)function;
  (void)value;
  (void)deviceName;
#endif
}

template <typename SetFunc>
VTKM_CONT void InitializeOption(RuntimeDeviceOption option,
                                SetFunc setFunc,
                                const std::string& funcName,
                                const std::string& deviceName)
{
  if (option.IsSet())
  {
    auto value = option.GetValue();
    auto code = setFunc(value);
    LogReturnCode(code, funcName, value, deviceName);
  }
}

} // namespace anonymous

RuntimeDeviceConfigurationBase::~RuntimeDeviceConfigurationBase() noexcept = default;

void RuntimeDeviceConfigurationBase::Initialize(
  const RuntimeDeviceConfigurationOptions& configOptions)
{
  InitializeOption(
    configOptions.VTKmNumThreads,
    [&](const vtkm::Id& value) { return this->SetThreads(value); },
    "SetThreads",
    this->GetDevice().GetName());
  InitializeOption(
    configOptions.VTKmNumaRegions,
    [&](const vtkm::Id& value) { return this->SetNumaRegions(value); },
    "SetNumaRegions",
    this->GetDevice().GetName());
  InitializeOption(
    configOptions.VTKmDeviceInstance,
    [&](const vtkm::Id& value) { return this->SetDeviceInstance(value); },
    "SetDeviceInstance",
    this->GetDevice().GetName());
  this->InitializeSubsystem();
}

void RuntimeDeviceConfigurationBase::Initialize(
  const RuntimeDeviceConfigurationOptions& configOptions,
  int& argc,
  char* argv[])
{
  this->ParseExtraArguments(argc, argv);
  this->Initialize(configOptions);
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::SetThreads(const vtkm::Id&)
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::SetNumaRegions(const vtkm::Id&)
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::SetDeviceInstance(const vtkm::Id&)
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

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::GetMaxThreads(vtkm::Id&) const
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::GetMaxDevices(vtkm::Id&) const
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

void RuntimeDeviceConfigurationBase::ParseExtraArguments(int&, char*[]) {}
void RuntimeDeviceConfigurationBase::InitializeSubsystem() {}


} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm
