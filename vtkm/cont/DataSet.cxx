//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Logging.h>

#include <algorithm>

namespace
{

VTKM_CONT void CheckFieldSize(const vtkm::cont::UnknownCellSet& cellSet,
                              const vtkm::cont::Field& field)
{
  if (!cellSet.IsValid())
  {
    return;
  }
  switch (field.GetAssociation())
  {
    case vtkm::cont::Field::Association::Points:
      if (cellSet.GetNumberOfPoints() != field.GetData().GetNumberOfValues())
      {
        VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                   "The size of field `"
                     << field.GetName() << "` (" << field.GetData().GetNumberOfValues()
                     << " values) does not match the size of the data set structure ("
                     << cellSet.GetNumberOfPoints() << " points).");
      }
      break;
    case vtkm::cont::Field::Association::Cells:
      if (cellSet.GetNumberOfCells() != field.GetData().GetNumberOfValues())
      {
        VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                   "The size of field `"
                     << field.GetName() << "` (" << field.GetData().GetNumberOfValues()
                     << " values) does not match the size of the data set structure ("
                     << cellSet.GetNumberOfCells() << " cells).");
      }
      break;
    default:
      // Ignore as the association does not match any topological element.
      break;
  }
}

VTKM_CONT void CheckFieldSizes(const vtkm::cont::UnknownCellSet& cellSet,
                               const vtkm::cont::internal::FieldCollection& fields)
{
  vtkm::IdComponent numFields = fields.GetNumberOfFields();
  for (vtkm::IdComponent fieldIndex = 0; fieldIndex < numFields; ++fieldIndex)
  {
    CheckFieldSize(cellSet, fields.GetField(fieldIndex));
  }
}

} // anonymous namespace

namespace vtkm
{
namespace cont
{

VTKM_CONT std::string& GlobalGhostCellFieldName() noexcept
{
  static std::string GhostCellName("vtkGhostCells");
  return GhostCellName;
}

VTKM_CONT const std::string& GetGlobalGhostCellFieldName() noexcept
{
  return GlobalGhostCellFieldName();
}

VTKM_CONT void SetGlobalGhostCellFieldName(const std::string& name) noexcept
{
  GlobalGhostCellFieldName() = name;
}

void DataSet::Clear()
{
  this->CoordSystemNames.clear();
  this->Fields.Clear();
  this->CellSet = this->CellSet.NewInstance();
}

void DataSet::AddField(const Field& field)
{
  CheckFieldSize(this->CellSet, field);
  this->Fields.AddField(field);
}

vtkm::Id DataSet::GetNumberOfCells() const
{
  return this->CellSet.GetNumberOfCells();
}

vtkm::Id DataSet::GetNumberOfPoints() const
{
  if (this->CellSet.IsValid())
  {
    return this->CellSet.GetNumberOfPoints();
  }

  // If there is no cell set, then try to use a coordinate system to get the number
  // of points.
  if (this->GetNumberOfCoordinateSystems() > 0)
  {
    return this->GetCoordinateSystem().GetNumberOfPoints();
  }

  // If there is no coordinate system either, we can try to guess the number of
  // points by finding a point field.
  for (vtkm::IdComponent fieldIdx = 0; fieldIdx < this->Fields.GetNumberOfFields(); ++fieldIdx)
  {
    const vtkm::cont::Field& field = this->Fields.GetField(fieldIdx);
    if (field.GetAssociation() == vtkm::cont::Field::Association::Points)
    {
      return field.GetData().GetNumberOfValues();
    }
  }

  // There are no point fields either.
  return 0;
}

const std::string& DataSet::GetGhostCellFieldName() const
{
  if (this->GhostCellName)
  {
    return *this->GhostCellName;
  }
  else
  {
    return GetGlobalGhostCellFieldName();
  }
}

bool DataSet::HasGhostCellField() const
{
  return this->HasCellField(this->GetGhostCellFieldName());
}

const vtkm::cont::Field& DataSet::GetGhostCellField() const
{
  if (this->HasGhostCellField())
  {
    return this->GetCellField(this->GetGhostCellFieldName());
  }
  else
  {
    throw vtkm::cont::ErrorBadValue("No Ghost Cell Field");
  }
}

vtkm::IdComponent DataSet::AddCoordinateSystem(const vtkm::cont::CoordinateSystem& cs)
{
  this->AddField(cs);
  return this->AddCoordinateSystem(cs.GetName());
}

vtkm::IdComponent DataSet::AddCoordinateSystem(const std::string& pointFieldName)
{
  // Check to see if we already have this coordinate system.
  vtkm::IdComponent index = this->GetCoordinateSystemIndex(pointFieldName);
  if (index >= 0)
  {
    return index;
  }

  // Check to make sure this is a valid point field.
  if (!this->HasPointField(pointFieldName))
  {
    throw vtkm::cont::ErrorBadValue("Cannot set point field named `" + pointFieldName +
                                    "` as a coordinate system because it does not exist.");
  }

  // Add the field to the list of coordinates.
  this->CoordSystemNames.push_back(pointFieldName);
  return static_cast<vtkm::IdComponent>(this->CoordSystemNames.size() - 1);
}

void DataSet::SetCellSetImpl(const vtkm::cont::UnknownCellSet& cellSet)
{
  CheckFieldSizes(cellSet, this->Fields);
  this->CellSet = cellSet;
}

void DataSet::SetGhostCellFieldName(const std::string& name)
{
  this->GhostCellName.reset(new std::string(name));
}

void DataSet::SetGhostCellField(const std::string& name)
{
  if (this->HasCellField(name))
  {
    this->SetGhostCellFieldName(name);
  }
  else
  {
    throw vtkm::cont::ErrorBadValue("No such cell field " + name);
  }
}

void DataSet::SetGhostCellField(const vtkm::cont::Field& field)
{
  if (field.GetAssociation() == vtkm::cont::Field::Association::Cells)
  {
    this->SetGhostCellField(field.GetName(), field.GetData());
  }
  else
  {
    throw vtkm::cont::ErrorBadValue("A ghost cell field must be a cell field.");
  }
}

void DataSet::SetGhostCellField(const std::string& fieldName,
                                const vtkm::cont::UnknownArrayHandle& field)
{
  this->AddCellField(fieldName, field);
  this->SetGhostCellField(fieldName);
}

void DataSet::SetGhostCellField(const vtkm::cont::UnknownArrayHandle& field)
{
  this->SetGhostCellField(GetGlobalGhostCellFieldName(), field);
}

void DataSet::CopyStructure(const vtkm::cont::DataSet& source)
{
  // Copy the cells.
  this->CellSet = source.CellSet;

  // Copy the coordinate systems.
  this->CoordSystemNames.clear();
  vtkm::IdComponent numCoords = source.GetNumberOfCoordinateSystems();
  for (vtkm::IdComponent coordIndex = 0; coordIndex < numCoords; ++coordIndex)
  {
    this->AddCoordinateSystem(source.GetCoordinateSystem(coordIndex));
  }

  // Copy the ghost cells.
  // Note that we copy the GhostCellName separately from the field it points to
  // to preserve (or remove) the case where the ghost cell name follows the
  // global ghost cell name.
  this->GhostCellName = source.GhostCellName;
  if (source.HasGhostCellField())
  {
    this->AddField(source.GetGhostCellField());
  }

  CheckFieldSizes(this->CellSet, this->Fields);
}

vtkm::cont::CoordinateSystem DataSet::GetCoordinateSystem(vtkm::Id index) const
{
  VTKM_ASSERT((index >= 0) && (index < this->GetNumberOfCoordinateSystems()));
  return this->GetPointField(this->CoordSystemNames[static_cast<std::size_t>(index)]);
}

vtkm::IdComponent DataSet::GetCoordinateSystemIndex(const std::string& name) const
{
  auto nameIter = std::find(this->CoordSystemNames.begin(), this->CoordSystemNames.end(), name);
  if (nameIter != this->CoordSystemNames.end())
  {
    return static_cast<vtkm::IdComponent>(std::distance(this->CoordSystemNames.begin(), nameIter));
  }
  else
  {
    return -1;
  }
}

const std::string& DataSet::GetCoordinateSystemName(vtkm::Id index) const
{
  VTKM_ASSERT((index >= 0) && (index < this->GetNumberOfCoordinateSystems()));
  return this->CoordSystemNames[static_cast<std::size_t>(index)];
}

vtkm::cont::CoordinateSystem DataSet::GetCoordinateSystem(const std::string& name) const
{
  vtkm::Id index = this->GetCoordinateSystemIndex(name);
  if (index < 0)
  {
    std::string error_message("No coordinate system with the name " + name +
                              " valid names are: \n");
    for (const auto& csn : this->CoordSystemNames)
    {
      error_message += csn + "\n";
    }
    throw vtkm::cont::ErrorBadValue(error_message);
  }
  return this->GetCoordinateSystem(index);
}

void DataSet::PrintSummary(std::ostream& out) const
{
  out << "DataSet:\n";
  out << "  CoordSystems[" << this->CoordSystemNames.size() << "]\n";
  out << "   ";
  for (const auto& csn : this->CoordSystemNames)
  {
    out << " " << csn;
  }
  out << "\n";

  out << "  CellSet \n";
  this->GetCellSet().PrintSummary(out);

  out << "  Fields[" << this->GetNumberOfFields() << "]\n";
  for (vtkm::Id index = 0; index < this->GetNumberOfFields(); index++)
  {
    this->GetField(index).PrintSummary(out);
  }

  out.flush();
}

void DataSet::ConvertToExpected()
{
  for (vtkm::IdComponent coordIndex = 0; coordIndex < this->GetNumberOfCoordinateSystems();
       ++coordIndex)
  {
    this->GetCoordinateSystem(coordIndex).ConvertToExpected();
  }

  for (vtkm::IdComponent fieldIndex = 0; fieldIndex < this->GetNumberOfFields(); ++fieldIndex)
  {
    this->GetField(fieldIndex).ConvertToExpected();
  }
}

} // namespace cont
} // namespace vtkm
