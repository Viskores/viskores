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
  this->CoordSystems.clear();
  this->Fields.Clear();
  this->CellSet = this->CellSet.NewInstance();
}

vtkm::Id DataSet::GetNumberOfCells() const
{
  return this->CellSet.GetNumberOfCells();
}

vtkm::Id DataSet::GetNumberOfPoints() const
{
  if (this->CoordSystems.empty())
  {
    return 0;
  }
  return this->CoordSystems[0].GetNumberOfPoints();
}

void DataSet::CopyStructure(const vtkm::cont::DataSet& source)
{
  this->CoordSystems = source.CoordSystems;
  this->CellSet = source.CellSet;
  this->GhostCellName = source.GhostCellName;
}

const vtkm::cont::CoordinateSystem& DataSet::GetCoordinateSystem(vtkm::Id index) const
{
  VTKM_ASSERT((index >= 0) && (index < this->GetNumberOfCoordinateSystems()));
  return this->CoordSystems[static_cast<std::size_t>(index)];
}

vtkm::cont::CoordinateSystem& DataSet::GetCoordinateSystem(vtkm::Id index)
{
  VTKM_ASSERT((index >= 0) && (index < this->GetNumberOfCoordinateSystems()));
  return this->CoordSystems[static_cast<std::size_t>(index)];
}

vtkm::Id DataSet::GetCoordinateSystemIndex(const std::string& name) const
{
  vtkm::Id index = -1;
  for (auto i = this->CoordSystems.begin(); i != this->CoordSystems.end(); ++i)
  {
    if (i->GetName() == name)
    {
      index = static_cast<vtkm::Id>(std::distance(this->CoordSystems.begin(), i));
      break;
    }
  }
  return index;
}

const vtkm::cont::CoordinateSystem& DataSet::GetCoordinateSystem(const std::string& name) const
{
  vtkm::Id index = this->GetCoordinateSystemIndex(name);
  if (index < 0)
  {
    std::string error_message("No coordinate system with the name " + name +
                              " valid names are: \n");
    for (const auto& cs : this->CoordSystems)
    {
      error_message += cs.GetName() + "\n";
    }
    throw vtkm::cont::ErrorBadValue(error_message);
  }
  return this->GetCoordinateSystem(index);
}

vtkm::cont::CoordinateSystem& DataSet::GetCoordinateSystem(const std::string& name)
{
  vtkm::Id index = this->GetCoordinateSystemIndex(name);
  if (index < 0)
  {
    std::string error_message("No coordinate system with the name " + name +
                              " valid names are: \n");
    for (const auto& cs : this->CoordSystems)
    {
      error_message += cs.GetName() + "\n";
    }
    throw vtkm::cont::ErrorBadValue(error_message);
  }
  return this->GetCoordinateSystem(index);
}

void DataSet::PrintSummary(std::ostream& out) const
{
  out << "DataSet:\n";
  out << "  CoordSystems[" << this->CoordSystems.size() << "]\n";
  for (std::size_t index = 0; index < this->CoordSystems.size(); index++)
  {
    this->CoordSystems[index].PrintSummary(out);
  }

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
