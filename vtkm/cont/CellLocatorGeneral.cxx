//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/CellLocatorGeneral.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellLocatorRectilinearGrid.h>
#include <vtkm/cont/CellLocatorUniformBins.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/CellSetStructured.h>

namespace
{

VTKM_CONT
void DefaultConfigurator(std::unique_ptr<vtkm::cont::CellLocator>& locator,
                         const vtkm::cont::DynamicCellSet& cellSet,
                         const vtkm::cont::CoordinateSystem& coords)
{
  using StructuredCellSet = vtkm::cont::CellSetStructured<3>;
  using UniformCoordinates = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using RectilinearCoordinates =
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>;

  if (cellSet.IsType<StructuredCellSet>() && coords.GetData().IsType<UniformCoordinates>())
  {
    if (!dynamic_cast<vtkm::cont::CellLocatorUniformGrid*>(locator.get()))
    {
      locator.reset(new vtkm::cont::CellLocatorUniformGrid);
    }
  }
  else if (cellSet.IsType<StructuredCellSet>() && coords.GetData().IsType<RectilinearCoordinates>())
  {
    if (!dynamic_cast<vtkm::cont::CellLocatorRectilinearGrid*>(locator.get()))
    {
      locator.reset(new vtkm::cont::CellLocatorRectilinearGrid);
    }
  }
  else if (!dynamic_cast<vtkm::cont::CellLocatorUniformBins*>(locator.get()))
  {
    locator.reset(new vtkm::cont::CellLocatorUniformBins);
  }

  locator->SetCellSet(cellSet);
  locator->SetCoordinates(coords);
}

} // anonymous namespace

namespace vtkm
{
namespace cont
{

VTKM_CONT CellLocatorGeneral::CellLocatorGeneral()
  : Configurator(DefaultConfigurator)
{
}

VTKM_CONT CellLocatorGeneral::~CellLocatorGeneral() = default;

VTKM_CONT const vtkm::exec::CellLocator* CellLocatorGeneral::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device) const
{
  if (this->Locator)
  {
    return this->Locator->PrepareForExecution(device);
  }
  return nullptr;
}

VTKM_CONT void CellLocatorGeneral::Build()
{
  this->Configurator(this->Locator, this->GetCellSet(), this->GetCoordinates());
  this->Locator->Update();
}

VTKM_CONT void CellLocatorGeneral::ResetToDefaultConfigurator()
{
  this->SetConfigurator(DefaultConfigurator);
}
}
} // vtkm::cont
