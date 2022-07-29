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
#include <vtkm/cont/CellLocatorTwoLevel.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/CellSetStructured.h>

namespace
{

template <typename LocatorImplType, typename LocatorVariantType>
void BuildForType(vtkm::cont::CellLocatorGeneral& locator, LocatorVariantType& locatorVariant)
{
  constexpr vtkm::IdComponent LOCATOR_INDEX =
    LocatorVariantType::template GetIndexOf<LocatorImplType>();
  if (locatorVariant.GetIndex() != LOCATOR_INDEX)
  {
    locatorVariant = LocatorImplType{};
  }
  LocatorImplType& locatorImpl = locatorVariant.template Get<LOCATOR_INDEX>();
  locatorImpl.SetCellSet(locator.GetCellSet());
  locatorImpl.SetCoordinates(locator.GetCoordinates());
  locatorImpl.Update();
}

} // anonymous namespace

namespace vtkm
{
namespace cont
{

VTKM_CONT void CellLocatorGeneral::Build()
{
  using StructuredCellSet = vtkm::cont::CellSetStructured<3>;
  using UniformCoordinates = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using RectilinearCoordinates =
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>;

  vtkm::cont::UnknownCellSet cellSet = this->GetCellSet();
  vtkm::cont::CoordinateSystem coords = this->GetCoordinates();

  if (cellSet.IsType<StructuredCellSet>() && coords.GetData().IsType<UniformCoordinates>())
  {
    BuildForType<vtkm::cont::CellLocatorUniformGrid>(*this, this->LocatorImpl);
  }
  else if (cellSet.IsType<StructuredCellSet>() && coords.GetData().IsType<RectilinearCoordinates>())
  {
    BuildForType<vtkm::cont::CellLocatorRectilinearGrid>(*this, this->LocatorImpl);
  }
  else
  {
    BuildForType<vtkm::cont::CellLocatorTwoLevel>(*this, this->LocatorImpl);
  }
}

struct CellLocatorGeneral::PrepareFunctor
{
  template <typename LocatorType>
  ExecObjType operator()(LocatorType&& locator,
                         vtkm::cont::DeviceAdapterId device,
                         vtkm::cont::Token& token) const
  {
    return locator.PrepareForExecution(device, token);
  }
};

CellLocatorGeneral::ExecObjType CellLocatorGeneral::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device,
  vtkm::cont::Token& token) const
{
  this->Update();
  return this->LocatorImpl.CastAndCall(PrepareFunctor{}, device, token);
}

}
} // vtkm::cont
