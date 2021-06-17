//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_RuntimeDeviceInformation_h
#define vtk_m_cont_RuntimeDeviceInformation_h

#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/internal/DeviceAdapterMemoryManager.h>
#include <vtkm/cont/internal/RuntimeDeviceConfiguration.h>
#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{
namespace cont
{

/// A class that can be used to determine if a given device adapter
/// is supported on the current machine at runtime. This is very important
/// for device adapters where a physical hardware requirements such as a GPU
/// or a Accelerator Card is needed for support to exist.
///
///
class VTKM_CONT_EXPORT RuntimeDeviceInformation
{
public:
  /// Returns the name corresponding to the device adapter id. If @a id is
  /// not recognized, `InvalidDeviceId` is returned. Queries for a
  /// name are all case-insensitive.
  VTKM_CONT
  DeviceAdapterNameType GetName(DeviceAdapterId id) const;

  /// Returns the id corresponding to the device adapter name. If @a name is
  /// not recognized, DeviceAdapterTagUndefined is returned.
  VTKM_CONT
  DeviceAdapterId GetId(DeviceAdapterNameType name) const;

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  VTKM_CONT
  bool Exists(DeviceAdapterId id) const;

  /// Returns a reference to a `DeviceAdapterMemoryManager` that will work with the
  /// given device. This method will throw an exception if the device id is not a
  /// real device (for example `DeviceAdapterTagAny`). If the device in question is
  /// not valid, a `DeviceAdapterMemoryManager` will be returned, but attempting to
  /// call any of the methods will result in a runtime exception.
  ///
  VTKM_CONT
  vtkm::cont::internal::DeviceAdapterMemoryManagerBase& GetMemoryManager(DeviceAdapterId id) const;

  /// Returns a reference to a `RuntimeDeviceConfiguration` that will work with the
  /// given device. If the device in question is not valid, a placeholder
  /// `InvalidRuntimeDeviceConfiguration` will be returned. Attempting to
  /// call any of the methods of this object will result in a runtime exception.
  /// The fully loaded version of this method is automatically called at the end
  /// of `vkmt::cont::Initialize` which performs automated setup of all runtime
  /// devices using parsed vtkm arguments.
  ///
  /// params:
  ///   id - The specific device to retreive the RuntimeDeviceConfiguration options for
  ///   configOptions - VTKm provided options that should be included when initializing
  ///                   a given RuntimeDeviceConfiguration
  ///   argc - The number of command line arguments to parse when Initializing
  ///          a given RuntimeDeviceConfiguration
  ///   argv - The extra command line arguments to parse when Initializing a given
  ///          RuntimeDeviceConfiguration. This argument is mainlued used in conjuction
  ///          with Kokkos config arg parsing to include specific --kokkos command
  ///          line flags and environment variables.
  VTKM_CONT
  vtkm::cont::internal::RuntimeDeviceConfigurationBase& GetRuntimeConfiguration(
    DeviceAdapterId id,
    const vtkm::cont::internal::RuntimeDeviceConfigurationOptions& configOptions,
    int& argc,
    char* argv[] = nullptr) const;

  VTKM_CONT
  vtkm::cont::internal::RuntimeDeviceConfigurationBase& GetRuntimeConfiguration(
    DeviceAdapterId id,
    const vtkm::cont::internal::RuntimeDeviceConfigurationOptions& configOptions) const;

  VTKM_CONT
  vtkm::cont::internal::RuntimeDeviceConfigurationBase& GetRuntimeConfiguration(
    DeviceAdapterId id) const;
};
} // namespace vtkm::cont
} // namespace vtkm

#endif //vtk_m_cont_RuntimeDeviceInformation_h
