//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace cont
{

DataSet::DataSet()
{
}

void DataSet::Clear()
{
  this->CoordSystems.clear();
  this->Fields.clear();
  this->CellSets.clear();
}

void DataSet::CopyStructure(const vtkm::cont::DataSet& source)
{
  this->CoordSystems = source.CoordSystems;
  this->CellSets = source.CellSets;
}

const vtkm::cont::Field& DataSet::GetField(vtkm::Id index) const
{
  VTKM_ASSERT((index >= 0) && (index < this->GetNumberOfFields()));
  return this->Fields[static_cast<std::size_t>(index)];
}

vtkm::Id DataSet::GetFieldIndex(const std::string& name, vtkm::cont::Field::Association assoc) const
{
  bool found;
  vtkm::Id index = this->FindFieldIndex(name, assoc, found);
  if (found)
  {
    return index;
  }
  else
  {
    throw vtkm::cont::ErrorBadValue("No field with requested name: " + name);
  }
}

const vtkm::cont::CoordinateSystem& DataSet::GetCoordinateSystem(vtkm::Id index) const
{
  VTKM_ASSERT((index >= 0) && (index < this->GetNumberOfCoordinateSystems()));
  return this->CoordSystems[static_cast<std::size_t>(index)];
}

vtkm::Id DataSet::GetCoordinateSystemIndex(const std::string& name) const
{
  bool found;
  vtkm::Id index = this->FindCoordinateSystemIndex(name, found);
  if (found)
  {
    return index;
  }
  else
  {
    throw vtkm::cont::ErrorBadValue("No coordinate system with requested name");
  }
}

vtkm::Id DataSet::GetCellSetIndex(const std::string& name) const
{
  bool found;
  vtkm::Id index = this->FindCellSetIndex(name, found);
  if (found)
  {
    return index;
  }
  else
  {
    throw vtkm::cont::ErrorBadValue("No cell set with requested name");
  }
}

void DataSet::PrintSummary(std::ostream& out) const
{
  out << "DataSet:\n";
  out << "  CoordSystems[" << this->CoordSystems.size() << "]\n";
  for (std::size_t index = 0; index < this->CoordSystems.size(); index++)
  {
    this->CoordSystems[index].PrintSummary(out);
  }

  out << "  CellSets[" << this->GetNumberOfCellSets() << "]\n";
  for (vtkm::Id index = 0; index < this->GetNumberOfCellSets(); index++)
  {
    this->GetCellSet(index).PrintSummary(out);
  }

  out << "  Fields[" << this->GetNumberOfFields() << "]\n";
  for (vtkm::Id index = 0; index < this->GetNumberOfFields(); index++)
  {
    this->GetField(index).PrintSummary(out);
  }
}

vtkm::Id DataSet::FindFieldIndex(const std::string& name,
                                 vtkm::cont::Field::Association association,
                                 bool& found) const
{
  for (std::size_t index = 0; index < this->Fields.size(); ++index)
  {
    if ((association == vtkm::cont::Field::Association::ANY ||
         association == this->Fields[index].GetAssociation()) &&
        this->Fields[index].GetName() == name)
    {
      found = true;
      return static_cast<vtkm::Id>(index);
    }
  }
  found = false;
  return -1;
}


vtkm::Id DataSet::FindCoordinateSystemIndex(const std::string& name, bool& found) const
{
  for (std::size_t index = 0; index < this->CoordSystems.size(); ++index)
  {
    if (this->CoordSystems[index].GetName() == name)
    {
      found = true;
      return static_cast<vtkm::Id>(index);
    }
  }
  found = false;
  return -1;
}

vtkm::Id DataSet::FindCellSetIndex(const std::string& name, bool& found) const
{
  for (std::size_t index = 0; index < static_cast<size_t>(this->GetNumberOfCellSets()); ++index)
  {
    if (this->CellSets[index].GetName() == name)
    {
      found = true;
      return static_cast<vtkm::Id>(index);
    }
  }
  found = false;
  return -1;
}

} // namespace cont
} // namespace vtkm
