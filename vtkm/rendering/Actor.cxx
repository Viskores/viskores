//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/Actor.h>

#include <vtkm/Assert.h>
#include <vtkm/cont/TryExecute.h>

namespace vtkm {
namespace rendering {

struct Actor::InternalsType
{
  vtkm::cont::DynamicCellSet Cells;
  vtkm::cont::CoordinateSystem Coordinates;
  vtkm::cont::Field ScalarField;
  vtkm::rendering::ColorTable ColorTable;

  vtkm::Range ScalarRange;
  vtkm::Bounds SpatialBounds;

  VTKM_CONT
  InternalsType(const vtkm::cont::DynamicCellSet &cells,
                const vtkm::cont::CoordinateSystem &coordinates,
                const vtkm::cont::Field &scalarField,
                const vtkm::rendering::ColorTable &colorTable)
    : Cells(cells),
      Coordinates(coordinates),
      ScalarField(scalarField),
      ColorTable(colorTable)
  {  }
};

struct Actor::RangeFunctor
{
  vtkm::rendering::Actor::InternalsType *Internals;
  const vtkm::cont::CoordinateSystem &Coordinates;
  const vtkm::cont::Field &ScalarField;

  VTKM_CONT
  RangeFunctor(vtkm::rendering::Actor *self,
               const vtkm::cont::CoordinateSystem &coordinates,
               const vtkm::cont::Field &scalarField)
    : Internals(self->Internals.get()),
      Coordinates(coordinates),
      ScalarField(scalarField)
  {  }

  template<typename Device>
  VTKM_CONT
  bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    this->ScalarField.GetRange(&this->Internals->ScalarRange, Device());
    this->Internals->SpatialBounds = this->Coordinates.GetBounds(Device());

    return true;
  }
};

Actor::Actor(const vtkm::cont::DynamicCellSet &cells,
             const vtkm::cont::CoordinateSystem &coordinates,
             const vtkm::cont::Field &scalarField,
             const vtkm::rendering::ColorTable &colorTable)
  : Internals(new InternalsType(cells, coordinates, scalarField, colorTable))
{
  VTKM_ASSERT(scalarField.GetData().GetNumberOfComponents() == 1);

  RangeFunctor functor(this, coordinates, scalarField);
  vtkm::cont::TryExecute(functor);
}

void Actor::Render(vtkm::rendering::Mapper &mapper,
                   vtkm::rendering::Canvas &canvas,
                   const vtkm::rendering::Camera &camera) const
{
  mapper.SetCanvas(&canvas);
  mapper.SetActiveColorTable(this->Internals->ColorTable);
  mapper.RenderCells(this->Internals->Cells,
                     this->Internals->Coordinates,
                     this->Internals->ScalarField,
                     this->Internals->ColorTable,
                     camera,
                     this->Internals->ScalarRange);
}

const vtkm::cont::DynamicCellSet &Actor::GetCells() const
{
  return this->Internals->Cells;
}

const vtkm::cont::CoordinateSystem &Actor::GetCoordiantes() const
{
  return this->Internals->Coordinates;
}

const vtkm::cont::Field &Actor::GetScalarField() const
{
  return this->Internals->ScalarField;
}

const vtkm::rendering::ColorTable &Actor::GetColorTable() const
{
  return this->Internals->ColorTable;
}

const vtkm::Range &Actor::GetScalarRange() const
{
  return this->Internals->ScalarRange;
}

const vtkm::Bounds &Actor::GetSpatialBounds() const
{
  return this->Internals->SpatialBounds;
}

void Actor::SetScalarRange(const vtkm::Range &scalarRange)
{
  this->Internals->ScalarRange = scalarRange;
}

}
} // namespace vtkm::rendering
