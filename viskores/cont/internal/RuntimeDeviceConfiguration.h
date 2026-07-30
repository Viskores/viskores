//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_internal_RuntimeDeviceConfiguration_h
#define viskores_cont_internal_RuntimeDeviceConfiguration_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/internal/RuntimeDeviceConfigurationOptions.h>

#include <vector>

namespace viskores
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

/// @brief Superclass for all `RuntimeDeviceConfiguration` classes.
///
/// Every device adapter must provide a specialization of `RuntimeDeviceConfiguration`,
/// and every specialization must inherit from this class.
class VISKORES_CONT_EXPORT RuntimeDeviceConfigurationBase
{
public:
  VISKORES_CONT virtual ~RuntimeDeviceConfigurationBase() noexcept;

  /// Returns a `viskores::cont::DeviceAdapterId` for the device that the runtime
  /// configuration oversees.
  VISKORES_CONT virtual viskores::cont::DeviceAdapterId GetDevice() const = 0;

  /// Calls the various `Set*` methods in this class with the provided set of config
  /// options which can either be manually provided or automatically initialized
  /// from command line arguments and environment variables via viskores::cont::Initialize.
  /// Each `Set*` method is called only if the corresponding viskores option is set, and a
  /// warning is logged based on the value of the `RuntimeDeviceConfigReturnCode` returned
  /// via the `Set*` method.
  VISKORES_CONT void Initialize(const RuntimeDeviceConfigurationOptions& configOptions);
  VISKORES_CONT void Initialize(const RuntimeDeviceConfigurationOptions& configOptions,
                                int& argc,
                                char* argv[]);

  /// Attempts to set the number of threads to use for this device.
  /// Returns `INVALID_FOR_DEVICE` if the overridden device does not
  /// support setting this configuration.
  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode SetThreads(const viskores::Id& value);

  /// Attempts to set the device instance to use.
  /// On systems that support multiple devices, the device to use in the
  /// current system process can be selected.
  /// Returns `INVALID_FOR_DEVICE` if the overridden device does not
  /// support setting this configuration.
  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode SetDeviceInstance(const viskores::Id& value);

  /// Attempts to get the number of threads to use for this device.
  /// Returns `INVALID_FOR_DEVICE` if the overridden device does not
  /// support this parameter.
  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetThreads(viskores::Id& value) const;

  /// Attempts to get the device instance to use.
  /// On systems that support multiple devices, the device to use in the
  /// current system process can be selected.
  /// Returns `INVALID_FOR_DEVICE` if the overridden device does not
  /// support this parameter.
  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetDeviceInstance(viskores::Id& value) const;

  /// Provides the maximum value that can be used in `SetThreads`.
  /// Returns `INVALID_FOR_DEVICE` if the overridden device does not
  /// support this parameter.
  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetMaxThreads(viskores::Id& value) const;

  /// Provides the maximum value that can be used in `SetDeviceInstance`.
  /// Returns `INVALID_FOR_DEVICE` if the overridden device does not
  /// support this parameter.
  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetMaxDevices(viskores::Id& value) const;

protected:
  /// An overridden method that can be used to perform extra command line argument parsing
  /// for cases where a specific device may use additional command line arguments. At the
  /// moment Kokkos is the only device that overrides this method.
  /// Note: This method assumes that viskores arguments have already been parsed and removed
  ///       from argv.
  VISKORES_CONT virtual void ParseExtraArguments(int& argc, char* argv[]);

  /// An overridden method that can be used to perform extra initialization after Extra
  /// Arguments are parsed and the Initialized ConfigOptions are used to call the various
  /// Set* methods at the end of Initialize. Particularly useful when initializing
  /// additional subsystems (like Kokkos).
  VISKORES_CONT virtual void InitializeSubsystem();
};

template <typename DeviceAdapterTag>
class RuntimeDeviceConfiguration;

} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores

#endif // viskores_cont_internal_RuntimeDeviceConfiguration_h
