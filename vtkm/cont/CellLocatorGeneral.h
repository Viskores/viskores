//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellLocatorGeneral_h
#define vtk_m_cont_CellLocatorGeneral_h

#include <vtkm/cont/CellLocator.h>

#include <functional>
#include <memory>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT CellLocatorGeneral : public vtkm::cont::CellLocator
{
public:
  VTKM_CONT CellLocatorGeneral();

  VTKM_CONT ~CellLocatorGeneral() override;

  /// Get the current underlying cell locator
  ///
  VTKM_CONT const vtkm::cont::CellLocator* GetCurrentLocator() const { return this->Locator.get(); }

  /// A configurator can be provided to select an appropriate
  /// cell locator implementation and configure its parameters, based on the
  /// input cell set and cooridinates.
  /// If unset, a resonable default is used.
  ///
  using ConfiguratorSignature = void(std::unique_ptr<vtkm::cont::CellLocator>&,
                                     const vtkm::cont::DynamicCellSet&,
                                     const vtkm::cont::CoordinateSystem&);

  VTKM_CONT void SetConfigurator(const std::function<ConfiguratorSignature>& configurator)
  {
    this->Configurator = configurator;
  }

  VTKM_CONT const std::function<ConfiguratorSignature>& GetConfigurator() const
  {
    return this->Configurator;
  }

  VTKM_CONT void ResetToDefaultConfigurator();

  VTKM_CONT const vtkm::exec::CellLocator* PrepareForExecution(
    vtkm::cont::DeviceAdapterId device) const override;

protected:
  VTKM_CONT void Build() override;

private:
  std::unique_ptr<vtkm::cont::CellLocator> Locator;
  std::function<ConfiguratorSignature> Configurator;
};
}
} // vtkm::cont

#endif // vtk_m_cont_CellLocatorGeneral_h
