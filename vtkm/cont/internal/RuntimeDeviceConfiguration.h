//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_RuntimeDeviceConfiguration_h
#define vtk_m_cont_internal_RuntimeDeviceConfiguration_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/internal/RuntimeDeviceConfigurationOptions.h>

#include <vector>

namespace vtkm
{
namespace cont
{
namespace internal
{

class VTKM_CONT_EXPORT RuntimeDeviceConfigurationBase
{
public:
  VTKM_CONT virtual ~RuntimeDeviceConfigurationBase() noexcept;
  VTKM_CONT virtual vtkm::cont::DeviceAdapterId GetDevice() const = 0;

  VTKM_CONT virtual void Initialize(const RuntimeDeviceConfigurationOptions& configOptions) const;
  VTKM_CONT virtual void Initialize(const RuntimeDeviceConfigurationOptions& configOptions,
                                    int&,
                                    char*[]) const;

  VTKM_CONT virtual void SetThreads(const vtkm::Id&) const;
  VTKM_CONT virtual void SetNumaRegions(const vtkm::Id&) const;
  VTKM_CONT virtual void SetDeviceInstance(const vtkm::Id&) const;

  VTKM_CONT virtual vtkm::Id GetThreads() const;
  VTKM_CONT virtual vtkm::Id GetNumaRegions() const;
  VTKM_CONT virtual vtkm::Id GetDeviceInstance() const;
};

template <typename DeviceAdapterTag>
class RuntimeDeviceConfiguration;

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif // vtk_m_cont_internal_RuntimeDeviceConfiguration_h
