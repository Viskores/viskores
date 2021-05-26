//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_RuntimeDeviceConfigurationOptions_h
#define vtk_m_cont_internal_RuntimeDeviceConfigurationOptions_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/internal/OptionParserArguments.h>
#include <vtkm/cont/internal/RuntimeDeviceOption.h>

#include <vector>

namespace vtkm
{
namespace cont
{
namespace internal
{

class VTKM_CONT_EXPORT RuntimeDeviceConfigurationOptions
{
public:
  VTKM_CONT RuntimeDeviceConfigurationOptions();
  VTKM_CONT RuntimeDeviceConfigurationOptions(std::vector<option::Descriptor>& usage);

  VTKM_CONT virtual ~RuntimeDeviceConfigurationOptions() noexcept;

  VTKM_CONT void Initialize(const option::Option* options);
  VTKM_CONT bool IsInitialized() const;

  RuntimeDeviceOption VTKmNumThreads;
  RuntimeDeviceOption VTKmNumaRegions;
  RuntimeDeviceOption VTKmDeviceInstance;

private:
  bool Initialized;
};

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif // vtk_m_cont_internal_RuntimeDeviceConfigurationOptions_h
