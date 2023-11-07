//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/Actor.h>

#include <vtkm/Assert.h>
#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/FieldRangeCompute.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <utility>

namespace vtkm
{
namespace rendering
{

struct Actor::InternalsType
{
  vtkm::cont::UnknownCellSet Cells;
  vtkm::cont::CoordinateSystem Coordinates;
  vtkm::cont::Field ScalarField;
  vtkm::cont::ColorTable ColorTable;

  vtkm::Range ScalarRange;
  vtkm::Bounds SpatialBounds;

  vtkm::cont::PartitionedDataSet DataSet;
  std::string FieldName;

  VTKM_CONT
  InternalsType(const vtkm::cont::PartitionedDataSet partitionedDataSet,
                const std::string fieldName,
                const vtkm::rendering::Color& color)
    : Cells(partitionedDataSet.GetPartition(0).GetCellSet())
    , Coordinates(partitionedDataSet.GetPartition(0).GetCoordinateSystem())
    , ScalarField(partitionedDataSet.GetPartition(0).GetField(fieldName))
    , ColorTable(vtkm::Range{ 0, 1 }, color.Components, color.Components)
    , DataSet(partitionedDataSet)
    , FieldName(fieldName)
  {
  }

  VTKM_CONT
  InternalsType(const vtkm::cont::PartitionedDataSet partitionedDataSet,
                const std::string fieldName,
                const vtkm::cont::ColorTable& colorTable = vtkm::cont::ColorTable::Preset::Default)
    : Cells(partitionedDataSet.GetPartition(0).GetCellSet())
    , Coordinates(partitionedDataSet.GetPartition(0).GetCoordinateSystem())
    , ScalarField(partitionedDataSet.GetPartition(0).GetField(fieldName))
    , ColorTable(colorTable)
    , DataSet(partitionedDataSet)
    , FieldName(fieldName)
  {
  }

  VTKM_CONT
  InternalsType(const vtkm::cont::UnknownCellSet& cells,
                const vtkm::cont::CoordinateSystem& coordinates,
                const vtkm::cont::Field& scalarField,
                const vtkm::rendering::Color& color)
    : Cells(std::move(cells))
    , Coordinates(std::move(coordinates))
    , ScalarField(std::move(scalarField))
    , ColorTable(vtkm::Range{ 0, 1 }, color.Components, color.Components)
  {
  }

  VTKM_CONT
  InternalsType(vtkm::cont::UnknownCellSet cells,
                vtkm::cont::CoordinateSystem coordinates,
                vtkm::cont::Field scalarField,
                const vtkm::cont::ColorTable& colorTable = vtkm::cont::ColorTable::Preset::Default)
    : Cells(std::move(cells))
    , Coordinates(std::move(coordinates))
    , ScalarField(std::move(scalarField))
    , ColorTable(colorTable)
  {
  }
};

Actor::Actor(const vtkm::cont::DataSet dataSet, const std::string fieldName)
{
  vtkm::cont::PartitionedDataSet partitionedDataSet;
  partitionedDataSet.AppendPartition(dataSet);
  this->Internals = std::make_unique<InternalsType>(partitionedDataSet, fieldName);
  this->Init();
}

Actor::Actor(const vtkm::cont::DataSet dataSet,
             const std::string fieldName,
             const vtkm::rendering::Color& color)
{
  vtkm::cont::PartitionedDataSet partitionedDataSet;
  partitionedDataSet.AppendPartition(dataSet);
  this->Internals = std::make_unique<InternalsType>(partitionedDataSet, fieldName, color);
  this->Init();
}

Actor::Actor(const vtkm::cont::DataSet dataSet,
             const std::string fieldName,
             const vtkm::cont::ColorTable& colorTable)
{
  vtkm::cont::PartitionedDataSet partitionedDataSet;
  partitionedDataSet.AppendPartition(dataSet);
  this->Internals = std::make_unique<InternalsType>(partitionedDataSet, fieldName, colorTable);
  this->Init();
}

Actor::Actor(const vtkm::cont::PartitionedDataSet dataSet, const std::string fieldName)
  : Internals(new InternalsType(dataSet, fieldName))
{
  this->Init();
}

Actor::Actor(const vtkm::cont::PartitionedDataSet dataSet,
             const std::string fieldName,
             const vtkm::rendering::Color& color)
  : Internals(new InternalsType(dataSet, fieldName, color))
{
  this->Init();
}

Actor::Actor(const vtkm::cont::PartitionedDataSet dataSet,
             const std::string fieldName,
             const vtkm::cont::ColorTable& colorTable)
  : Internals(new InternalsType(dataSet, fieldName, colorTable))
{
  this->Init();
}

Actor::Actor(const vtkm::cont::UnknownCellSet& cells,
             const vtkm::cont::CoordinateSystem& coordinates,
             const vtkm::cont::Field& scalarField)
  : Internals(std::make_unique<InternalsType>(cells, coordinates, scalarField))
{
  this->Init(coordinates, scalarField);
}

Actor::Actor(const vtkm::cont::UnknownCellSet& cells,
             const vtkm::cont::CoordinateSystem& coordinates,
             const vtkm::cont::Field& scalarField,
             const vtkm::rendering::Color& color)
  : Internals(std::make_unique<InternalsType>(cells, coordinates, scalarField, color))
{
  this->Init(coordinates, scalarField);
}

Actor::Actor(const vtkm::cont::UnknownCellSet& cells,
             const vtkm::cont::CoordinateSystem& coordinates,
             const vtkm::cont::Field& scalarField,
             const vtkm::cont::ColorTable& colorTable)
  : Internals(std::make_unique<InternalsType>(cells, coordinates, scalarField, colorTable))
{
  this->Init(coordinates, scalarField);
}

Actor::Actor(const Actor& rhs)
  : Internals(nullptr)
{
  // rhs might have been moved, its Internal would be nullptr
  if (rhs.Internals)
    Internals = std::make_unique<InternalsType>(*rhs.Internals);
}

Actor& Actor::operator=(const Actor& rhs)
{
  // both *this and rhs might have been moved.
  if (!rhs.Internals)
  {
    Internals.reset();
  }
  else if (!Internals)
  {
    Internals = std::make_unique<InternalsType>(*rhs.Internals);
  }
  else
  {
    *Internals = *rhs.Internals;
  }

  return *this;
}

Actor::Actor(vtkm::rendering::Actor&&) noexcept = default;
Actor& Actor::operator=(Actor&&) noexcept = default;
Actor::~Actor() = default;

void Actor::Init(const vtkm::cont::CoordinateSystem& coordinates,
                 const vtkm::cont::Field& scalarField)
{
  scalarField.GetRange(&this->Internals->ScalarRange);
  this->Internals->SpatialBounds = coordinates.GetBounds();
}

void Actor::Init()
{
  this->Internals->SpatialBounds = vtkm::cont::BoundsCompute(this->Internals->DataSet);
  this->Internals->ScalarRange =
    vtkm::cont::FieldRangeCompute(this->Internals->DataSet, this->Internals->FieldName)
      .ReadPortal()
      .Get(0);
}

void Actor::Render(vtkm::rendering::Mapper& mapper,
                   vtkm::rendering::Canvas& canvas,
                   const vtkm::rendering::Camera& camera) const
{
  mapper.SetCanvas(&canvas);
  mapper.SetActiveColorTable(this->Internals->ColorTable);
  if (this->Internals->DataSet.GetNumberOfPartitions() > 0)
  {
    mapper.RenderCellsPartitioned(this->Internals->DataSet,
                                  this->Internals->FieldName,
                                  this->Internals->ColorTable,
                                  camera,
                                  this->Internals->ScalarRange);
  }
  else
  {
    mapper.RenderCells(this->Internals->Cells,
                       this->Internals->Coordinates,
                       this->Internals->ScalarField,
                       this->Internals->ColorTable,
                       camera,
                       this->Internals->ScalarRange);
  }
}


const vtkm::cont::UnknownCellSet& Actor::GetCells() const
{
  return this->Internals->Cells;
}

const vtkm::cont::CoordinateSystem& Actor::GetCoordinates() const
{
  return this->Internals->Coordinates;
}

const vtkm::cont::Field& Actor::GetScalarField() const
{
  return this->Internals->ScalarField;
}

const vtkm::cont::ColorTable& Actor::GetColorTable() const
{
  return this->Internals->ColorTable;
}

const vtkm::Range& Actor::GetScalarRange() const
{
  return this->Internals->ScalarRange;
}

const vtkm::Bounds& Actor::GetSpatialBounds() const
{
  return this->Internals->SpatialBounds;
}

void Actor::SetScalarRange(const vtkm::Range& scalarRange)
{
  this->Internals->ScalarRange = scalarRange;
}
}
} // namespace vtkm::rendering
