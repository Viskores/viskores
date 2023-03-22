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

/// Provides a default set of RuntimeDeviceOptions that vtk-m currently supports setting.
/// Each option provided in this class should have a corresponding `Set*` method in the
/// RuntimeDeviceConfiguration.
class VTKM_CONT_EXPORT RuntimeDeviceConfigurationOptions
{
public:
  VTKM_CONT RuntimeDeviceConfigurationOptions();

  /// Calls the default constructor and additionally pushes back additional command line
  /// options to the provided usage vector for integration with the vtkm option parser.
  VTKM_CONT RuntimeDeviceConfigurationOptions(std::vector<option::Descriptor>& usage);

  /// Allows the caller to initialize these runtime config arguments directly from
  /// command line arguments
  VTKM_CONT RuntimeDeviceConfigurationOptions(int& argc, char* argv[]);

  VTKM_CONT virtual ~RuntimeDeviceConfigurationOptions() noexcept;

  /// Calls Initialize for each of this class's current configuration options and marks
  /// the options as initialized.
  VTKM_CONT void Initialize(const option::Option* options);
  VTKM_CONT bool IsInitialized() const;

  RuntimeDeviceOption VTKmNumThreads;
  RuntimeDeviceOption VTKmDeviceInstance;

protected:
  /// Sets the option indices and environment varaible names for the vtkm supported options.
  /// If useOptionIndex is set the OptionParserArguments enum for option indices will be used,
  /// otherwise ints from 0 - numOptions will be used.
  VTKM_CONT RuntimeDeviceConfigurationOptions(const bool& useOptionIndex);

private:
  bool Initialized;
};

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif // vtk_m_cont_internal_RuntimeDeviceConfigurationOptions_h
