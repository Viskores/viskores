//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_RuntimeDeviceOption_h
#define vtk_m_cont_internal_RuntimeDeviceOption_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/internal/OptionParser.h>
#include <vtkm/cont/internal/OptionParserArguments.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

enum class RuntimeDeviceOptionSource
{
  COMMAND_LINE,
  ENVIRONMENT,
  IN_CODE,
  NOT_SET
};

class VTKM_CONT_EXPORT RuntimeDeviceOption
{
public:
  VTKM_CONT RuntimeDeviceOption(const vtkm::Id& index, const std::string& envName);

  VTKM_CONT virtual ~RuntimeDeviceOption() noexcept;

  VTKM_CONT void Initialize(const option::Option* options);
  VTKM_CONT void SetOptionFromEnvironment();
  VTKM_CONT void SetOptionFromOptionsArray(const option::Option* options);
  VTKM_CONT void SetOption(const vtkm::Id& value);
  VTKM_CONT vtkm::Id GetValue() const;
  VTKM_CONT RuntimeDeviceOptionSource GetSource() const;
  VTKM_CONT bool IsSet() const;

private:
  const vtkm::Id Index;
  const std::string EnvName;
  RuntimeDeviceOptionSource Source;
  vtkm::Id Value;
};

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm

#endif // vtk_m_cont_internal_RuntimeDeviceOption_h
