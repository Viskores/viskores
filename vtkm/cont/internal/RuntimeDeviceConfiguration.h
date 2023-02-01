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
  INVALID_VALUE,
  NOT_APPLIED
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
  VTKM_CONT void Initialize(const RuntimeDeviceConfigurationOptions& configOptions);
  VTKM_CONT void Initialize(const RuntimeDeviceConfigurationOptions& configOptions,
                            int& argc,
                            char* argv[]);

  /// The following public methods should be overriden in each individual device.
  /// A method should return INVALID_FOR_DEVICE if the overriden device does not
  /// support the particular set method.
  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetThreads(const vtkm::Id& value);
  VTKM_CONT virtual RuntimeDeviceConfigReturnCode SetDeviceInstance(const vtkm::Id& value);

  /// The following public methods are overriden in each individual device and store the
  /// values that were set via the above Set* methods for the given device.
  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetThreads(vtkm::Id& value) const;
  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetDeviceInstance(vtkm::Id& value) const;

  /// The following public methods should be overriden as needed for each individual device
  /// as they describe various device parameters.
  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetMaxThreads(vtkm::Id& value) const;
  VTKM_CONT virtual RuntimeDeviceConfigReturnCode GetMaxDevices(vtkm::Id& value) const;

protected:
  /// An overriden method that can be used to perform extra command line argument parsing
  /// for cases where a specific device may use additional command line arguments. At the
  /// moment Kokkos is the only device that overrides this method.
  /// Note: This method assumes that vtk-m arguments have already been parsed and removed
  ///       from argv.
  VTKM_CONT virtual void ParseExtraArguments(int& argc, char* argv[]);

  /// An overriden method that can be used to perform extra initialization after Extra
  /// Arguments are parsed and the Initialized ConfigOptions are used to call the various
  /// Set* methods at the end of Initialize. Particuarly useful when initializing
  /// additional subystems (like Kokkos).
  VTKM_CONT virtual void InitializeSubsystem();
};

template <typename DeviceAdapterTag>
class RuntimeDeviceConfiguration;

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif // vtk_m_cont_internal_RuntimeDeviceConfiguration_h
