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
  vtkm::cont::PartitionedDataSet Data;
  std::string CoordinateName;
  std::string FieldName;
  vtkm::cont::Field::Association FieldAssociation;

  vtkm::cont::ColorTable ColorTable;

  vtkm::Range ScalarRange;
  vtkm::Bounds SpatialBounds;

  VTKM_CONT
  InternalsType(const vtkm::cont::PartitionedDataSet partitionedDataSet,
                const std::string coordinateName,
                const std::string fieldName,
                const vtkm::rendering::Color& color)
    : Data(partitionedDataSet)
    , CoordinateName(coordinateName)
    , FieldName(fieldName)
    , ColorTable(vtkm::Range{ 0, 1 }, color.Components, color.Components)
  {
  }

  VTKM_CONT
  InternalsType(const vtkm::cont::PartitionedDataSet partitionedDataSet,
                const std::string coordinateName,
                const std::string fieldName,
                const vtkm::cont::ColorTable& colorTable = vtkm::cont::ColorTable::Preset::Default)
    : Data(partitionedDataSet)
    , CoordinateName(coordinateName)
    , FieldName(fieldName)
    , ColorTable(colorTable)
  {
  }
};

Actor::Actor(const vtkm::cont::DataSet dataSet,
             const std::string coordinateName,
             const std::string fieldName)
{
  vtkm::cont::PartitionedDataSet partitionedDataSet(dataSet);
  this->Internals = std::make_unique<InternalsType>(partitionedDataSet, coordinateName, fieldName);
  this->Init();
}

Actor::Actor(const vtkm::cont::DataSet dataSet,
             const std::string coordinateName,
             const std::string fieldName,
             const vtkm::rendering::Color& color)
{
  vtkm::cont::PartitionedDataSet partitionedDataSet(dataSet);
  this->Internals =
    std::make_unique<InternalsType>(partitionedDataSet, coordinateName, fieldName, color);
  this->Init();
}

Actor::Actor(const vtkm::cont::DataSet dataSet,
             const std::string coordinateName,
             const std::string fieldName,
             const vtkm::cont::ColorTable& colorTable)
{
  vtkm::cont::PartitionedDataSet partitionedDataSet(dataSet);
  this->Internals =
    std::make_unique<InternalsType>(partitionedDataSet, coordinateName, fieldName, colorTable);
  this->Init();
}

Actor::Actor(const vtkm::cont::PartitionedDataSet dataSet,
             const std::string coordinateName,
             const std::string fieldName)
  : Internals(new InternalsType(dataSet, coordinateName, fieldName))
{
  this->Init();
}

Actor::Actor(const vtkm::cont::PartitionedDataSet dataSet,
             const std::string coordinateName,
             const std::string fieldName,
             const vtkm::rendering::Color& color)
  : Internals(new InternalsType(dataSet, coordinateName, fieldName, color))
{
  this->Init();
}

Actor::Actor(const vtkm::cont::PartitionedDataSet dataSet,
             const std::string coordinateName,
             const std::string fieldName,
             const vtkm::cont::ColorTable& colorTable)
  : Internals(new InternalsType(dataSet, coordinateName, fieldName, colorTable))
{
  this->Init();
}

Actor::Actor(const vtkm::cont::UnknownCellSet& cells,
             const vtkm::cont::CoordinateSystem& coordinates,
             const vtkm::cont::Field& scalarField)
{
  vtkm::cont::DataSet dataSet;
  dataSet.SetCellSet(cells);
  dataSet.AddCoordinateSystem(coordinates);
  dataSet.AddField(scalarField);
  this->Internals =
    std::make_unique<InternalsType>(dataSet, coordinates.GetName(), scalarField.GetName());
  this->Init();
}

Actor::Actor(const vtkm::cont::UnknownCellSet& cells,
             const vtkm::cont::CoordinateSystem& coordinates,
             const vtkm::cont::Field& scalarField,
             const vtkm::rendering::Color& color)
{
  vtkm::cont::DataSet dataSet;
  dataSet.SetCellSet(cells);
  dataSet.AddCoordinateSystem(coordinates);
  dataSet.AddField(scalarField);
  this->Internals =
    std::make_unique<InternalsType>(dataSet, coordinates.GetName(), scalarField.GetName(), color);
  this->Init();
}

Actor::Actor(const vtkm::cont::UnknownCellSet& cells,
             const vtkm::cont::CoordinateSystem& coordinates,
             const vtkm::cont::Field& scalarField,
             const vtkm::cont::ColorTable& colorTable)
{
  vtkm::cont::DataSet dataSet;
  dataSet.SetCellSet(cells);
  dataSet.AddCoordinateSystem(coordinates);
  dataSet.AddField(scalarField);
  this->Internals = std::make_unique<InternalsType>(
    dataSet, coordinates.GetName(), scalarField.GetName(), colorTable);
  this->Init();
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

void Actor::Init()
{
  this->Internals->SpatialBounds = vtkm::cont::BoundsCompute(this->Internals->Data);
  this->Internals->ScalarRange =
    vtkm::cont::FieldRangeCompute(this->Internals->Data, this->Internals->FieldName)
      .ReadPortal()
      .Get(0);
}

void Actor::Render(vtkm::rendering::Mapper& mapper,
                   vtkm::rendering::Canvas& canvas,
                   const vtkm::rendering::Camera& camera) const
{
  mapper.SetCanvas(&canvas);
  mapper.SetActiveColorTable(this->Internals->ColorTable);
  mapper.RenderCellsPartitioned(this->Internals->Data,
                                this->Internals->FieldName,
                                this->Internals->ColorTable,
                                camera,
                                this->Internals->ScalarRange);
}


const vtkm::cont::UnknownCellSet& Actor::GetCells() const
{
  return this->Internals->Data.GetPartition(0).GetCellSet();
}

vtkm::cont::CoordinateSystem Actor::GetCoordinates() const
{
  return this->Internals->Data.GetPartition(0).GetCoordinateSystem(this->Internals->CoordinateName);
}

const vtkm::cont::Field& Actor::GetScalarField() const
{
  return this->Internals->Data.GetPartition(0).GetField(this->Internals->FieldName);
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
