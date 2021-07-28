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

enum class RuntimeDeviceConfigReturnCode
{
  SUCCESS,
  OUT_OF_BOUNDS,
  INVALID_FOR_DEVICE,
  INVALID_VALUE
};

class VTKM_CONT_EXPORT RuntimeDeviceConfigurationBase
{
public:
  VTKM_CONT virtual ~RuntimeDeviceConfigurationBase() noexcept;
  VTKM_CONT virtual vtkm::cont::DeviceAdapterId GetDevice() const = 0;

  /// Calls the various `Set*` methods in this class with the provided set of config
  /// options which can either be manually provided or automatically initialized
  /// from command line arguments and environment variables via vtkm::cont::Initialize.
  /// Each `Set*` method is called only if the corresponding vtk-m option is set, and a
  /// warning is logged based on the value of the `RuntimeDeviceConfigReturnCode` returned
  /// via the `Set*` method.
  VTKM_CONT void Initialize(const RuntimeDeviceConfigurationOptions& configOptions) const;
  VTKM_CONT void Initialize(const RuntimeDeviceConfigurationOptions& configOptions,
                            int& argc,
                            char* argv[]) const;

  /// The following public methods should be overriden in each individual device.
  /// A method should return INVALID_FOR_DEVICE if the overriden device does not
  /// support the particular set method.
  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetThreads(const vtkm::Id&) const;
  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetNumaRegions(const vtkm::Id&) const;
  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetDeviceInstance(const vtkm::Id&) const;

  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetThreads(vtkm::Id& value) const;
  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetNumaRegions(vtkm::Id& value) const;
  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetDeviceInstance(vtkm::Id& value) const;

protected:
  /// An overriden method that can be used to perform extra command line argument parsing
  /// for cases where a specific device may use additional command line arguments. At the
  /// moment Kokkos is the only device that overrides this method.
  VTKM_CONT virtual void ParseExtraArguments(int&, char*[]) const;

  /// Used during Initialize to log a warning message dependent on the return code when
  /// calling a specific `Set*` method.
  ///
  /// params:
  ///   code - The code to log a message for
  ///   function - The name of the `Set*` function the code was returned from
  ///   value - The value used as the argument to the `Set*` call.
  VTKM_CONT virtual void LogReturnCode(const RuntimeDeviceConfigReturnCode& code,
                                       const std::string& function,
                                       const vtkm::Id& value) const;
};

template <typename DeviceAdapterTag>
class RuntimeDeviceConfiguration;

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif // vtk_m_cont_internal_RuntimeDeviceConfiguration_h
