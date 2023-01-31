//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/interop/anari/ANARIActor.h>

namespace vtkm
{
namespace interop
{
namespace anari
{

const char* AnariMaterialInputString(vtkm::IdComponent p)
{
  switch (p)
  {
    case 0:
    default:
      return "attribute0";
    case 1:
      return "attribute1";
    case 2:
      return "attribute2";
    case 3:
      return "attribute3";
  }

  return "attribute0";
}

ANARIActor::ANARIActor(const vtkm::cont::UnknownCellSet& cells,
                       const vtkm::cont::CoordinateSystem& coordinates,
                       const vtkm::cont::Field& field0,
                       const vtkm::cont::Field& field1,
                       const vtkm::cont::Field& field2,
                       const vtkm::cont::Field& field3)
{
  this->Data->Cells = cells;
  this->Data->Coordinates = coordinates;
  this->Data->Fields[0] = field0;
  this->Data->Fields[1] = field1;
  this->Data->Fields[2] = field2;
  this->Data->Fields[3] = field3;
}

ANARIActor::ANARIActor(const vtkm::cont::UnknownCellSet& cells,
                       const vtkm::cont::CoordinateSystem& coordinates,
                       const FieldSet& f)
  : ANARIActor(cells, coordinates, f[0], f[1], f[2], f[3])
{
}

ANARIActor::ANARIActor(const vtkm::cont::DataSet& dataset,
                       const std::string& field0,
                       const std::string& field1,
                       const std::string& field2,
                       const std::string& field3)
{
  this->Data->Cells = dataset.GetCellSet();
  if (dataset.GetNumberOfCoordinateSystems() > 0)
    this->Data->Coordinates = dataset.GetCoordinateSystem();
  this->Data->Fields[0] = field0.empty() ? vtkm::cont::Field{} : dataset.GetField(field0);
  this->Data->Fields[1] = field1.empty() ? vtkm::cont::Field{} : dataset.GetField(field1);
  this->Data->Fields[2] = field2.empty() ? vtkm::cont::Field{} : dataset.GetField(field2);
  this->Data->Fields[3] = field3.empty() ? vtkm::cont::Field{} : dataset.GetField(field3);
}

const vtkm::cont::UnknownCellSet& ANARIActor::GetCellSet() const
{
  return this->Data->Cells;
}

const vtkm::cont::CoordinateSystem& ANARIActor::GetCoordinateSystem() const
{
  return this->Data->Coordinates;
}

const vtkm::cont::Field& ANARIActor::GetField(vtkm::IdComponent idx) const
{
  return this->Data->Fields[idx < 0 ? GetPrimaryFieldIndex() : idx];
}

FieldSet ANARIActor::GetFieldSet() const
{
  return this->Data->Fields;
}

void ANARIActor::SetPrimaryFieldIndex(vtkm::IdComponent idx)
{
  this->Data->PrimaryField = idx;
}

vtkm::IdComponent ANARIActor::GetPrimaryFieldIndex() const
{
  return this->Data->PrimaryField;
}

vtkm::cont::DataSet ANARIActor::MakeDataSet(bool includeFields) const
{
  vtkm::cont::DataSet dataset;
  dataset.SetCellSet(GetCellSet());
  dataset.AddCoordinateSystem(GetCoordinateSystem());
  if (!includeFields)
    return dataset;

  auto addField = [&](const vtkm::cont::Field& field) {
    if (field.GetNumberOfValues() > 0)
      dataset.AddField(field);
  };

  addField(this->Data->Fields[0]);
  addField(this->Data->Fields[1]);
  addField(this->Data->Fields[2]);
  addField(this->Data->Fields[3]);

  return dataset;
}

} // namespace anari
} // namespace interop
} // namespace vtkm
