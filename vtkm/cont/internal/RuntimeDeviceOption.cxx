//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/internal/RuntimeDeviceOption.h>

#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Logging.h>

#include <string>

namespace vtkm
{
namespace cont
{
namespace internal
{

namespace
{

VTKM_CONT vtkm::Id ParseOption(const std::string& input, const std::string& source)
{
  try
  {
    size_t pos;
    auto value = std::stoi(input, &pos, 10);
    if (pos != input.size())
    {
      throw vtkm::cont::ErrorBadValue("Value '" + input + "' from source: '" + source +
                                      "' has dangling characters, throwing");
    }
    return value;
  }
  catch (const std::invalid_argument&)
  {
    throw vtkm::cont::ErrorBadValue("Value '" + input +
                                    "' failed to parse as integer from source: '" + source + "'");
  }
  catch (const std::out_of_range&)
  {
    throw vtkm::cont::ErrorBadValue("Value '" + input + "' out of range for source: '" + source +
                                    "'");
  }
}

} // namespace

RuntimeDeviceOption::RuntimeDeviceOption(const vtkm::Id& index, const std::string& envName)
  : Index(index)
  , EnvName(envName)
  , Source(RuntimeDeviceOptionSource::NOT_SET)
{
}

RuntimeDeviceOption::~RuntimeDeviceOption() noexcept = default;

void RuntimeDeviceOption::Initialize(const option::Option* options)
{
  this->SetOptionFromEnvironment();
  this->SetOptionFromOptionsArray(options);
}

void RuntimeDeviceOption::SetOptionFromEnvironment()
{
  if (std::getenv(EnvName.c_str()) != nullptr)
  {
    this->Value = ParseOption(std::getenv(EnvName.c_str()), "ENVIRONMENT: " + EnvName);
    this->Source = RuntimeDeviceOptionSource::ENVIRONMENT;
  }
}

void RuntimeDeviceOption::SetOptionFromOptionsArray(const option::Option* options)
{
  if (options != nullptr && options[this->Index])
  {
    this->Value = ParseOption(options[this->Index].arg,
                              "COMMAND_LINE: " + std::string{ options[this->Index].name });
    this->Source = RuntimeDeviceOptionSource::COMMAND_LINE;
  }
}

void RuntimeDeviceOption::SetOption(const vtkm::Id& value)
{
  this->Value = value;
  this->Source = RuntimeDeviceOptionSource::IN_CODE;
}

vtkm::Id RuntimeDeviceOption::GetValue() const
{
  if (!this->IsSet())
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               "GetValue() called on Argument '" << this->EnvName << "' when it was not set.");
  }
  return this->Value;
}

RuntimeDeviceOptionSource RuntimeDeviceOption::GetSource() const
{
  return this->Source;
}

bool RuntimeDeviceOption::IsSet() const
{
  return this->Source != RuntimeDeviceOptionSource::NOT_SET;
}

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm
