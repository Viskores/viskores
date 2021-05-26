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

#include <vtkm/cont/Logging.h>

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
    this->SetThreads(configOptions.VTKmNumThreads.GetValue());
  }
  if (configOptions.VTKmNumaRegions.IsSet())
  {
    this->SetNumaRegions(configOptions.VTKmNumaRegions.GetValue());
  }
  if (configOptions.VTKmDeviceInstance.IsSet())
  {
    this->SetDeviceInstance(configOptions.VTKmDeviceInstance.GetValue());
  }
}

void RuntimeDeviceConfigurationBase::Initialize(
  const RuntimeDeviceConfigurationOptions& configOptions,
  int&,
  char*[]) const
{
  this->Initialize(configOptions);
}

void RuntimeDeviceConfigurationBase::SetThreads(const vtkm::Id&) const
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
             "Called 'SetThreads' for unsupported Device '" << this->GetDevice().GetName()
                                                            << "', no-op");
}

void RuntimeDeviceConfigurationBase::SetNumaRegions(const vtkm::Id&) const
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
             "Called 'SetNumaRegions' for unsupported Device '" << this->GetDevice().GetName()
                                                                << "', no-op");
}

void RuntimeDeviceConfigurationBase::SetDeviceInstance(const vtkm::Id&) const
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
             "Called 'SetDeviceInstance' for unsupported Device '" << this->GetDevice().GetName()
                                                                   << "', no-op");
}

vtkm::Id RuntimeDeviceConfigurationBase::GetThreads() const
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
             "Called 'GetThreads' for unsupported Device '" << this->GetDevice().GetName()
                                                            << "', returning -1");
  return -1;
}

vtkm::Id RuntimeDeviceConfigurationBase::GetNumaRegions() const
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
             "Called 'GetNumaRegions' for unsupported Device '" << this->GetDevice().GetName()
                                                                << "', returning -1");
  return -1;
}

vtkm::Id RuntimeDeviceConfigurationBase::GetDeviceInstance() const
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
             "Called 'GetDeviceInstance' for unsupported Device '" << this->GetDevice().GetName()
                                                                   << "', returning -1");
  return -1;
}

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm
