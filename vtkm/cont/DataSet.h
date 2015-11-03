//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_DataSet_h
#define vtk_m_cont_DataSet_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/cont/Field.h>

namespace vtkm {
namespace cont {

class DataSet
{
public:
  VTKM_CONT_EXPORT
  DataSet()
  {
  }

  VTKM_CONT_EXPORT
  void Clear()
  {
    this->CoordSystems.clear();
    this->Fields.clear();
    this->CellSets.clear();
  }

  VTKM_CONT_EXPORT
  void AddField(Field field)
  {
    this->Fields.push_back(field);
  }

  VTKM_CONT_EXPORT
  const vtkm::cont::Field &GetField(vtkm::Id index) const
  {
    VTKM_ASSERT_CONT((index >= 0) &&
                     (index < this->GetNumberOfFields()));
    return this->Fields[static_cast<std::size_t>(index)];
  }

  VTKM_CONT_EXPORT
  const vtkm::cont::Field &GetField(const std::string &name,
      vtkm::cont::Field::AssociationEnum assoc = vtkm::cont::Field::ASSOC_ANY)
      const
  {
    for (std::size_t i=0; i < this->Fields.size(); ++i)
    {
      if ((assoc == vtkm::cont::Field::ASSOC_ANY ||
           assoc == this->Fields[i].GetAssociation()) &&
          this->Fields[i].GetName() == name)
      {
        return this->Fields[i];
      }
    }
    throw vtkm::cont::ErrorControlBadValue("No field with requested name");
  }

  VTKM_CONT_EXPORT
  void AddCoordinateSystem(vtkm::cont::CoordinateSystem cs)
  {
    this->CoordSystems.push_back(cs);
  }

  VTKM_CONT_EXPORT
  const vtkm::cont::CoordinateSystem &
  GetCoordinateSystem(vtkm::Id index=0) const
  {
    VTKM_ASSERT_CONT((index >= 0) &&
                     (index < this->GetNumberOfCoordinateSystems()));
    return this->CoordSystems[static_cast<std::size_t>(index)];
  }

  VTKM_CONT_EXPORT
  const vtkm::cont::CoordinateSystem &
  GetCoordinateSystem(const std::string &name) const
  {
    for (std::size_t i=0; i < this->CoordSystems.size(); ++i)
    {
      if (this->CoordSystems[i].GetName() == name)
      {
        return this->CoordSystems[i];
      }
    }
    throw vtkm::cont::ErrorControlBadValue(
          "No coordinate system with requested name");
  }

  VTKM_CONT_EXPORT
  void AddCellSet(vtkm::cont::DynamicCellSet cellSet)
  {
    this->CellSets.push_back(cellSet);
  }

  template<typename CellSetType>
  VTKM_CONT_EXPORT
  void AddCellSet(const CellSetType &cellSet)
  {
    VTKM_IS_CELL_SET(CellSetType);
    this->CellSets.push_back(vtkm::cont::DynamicCellSet(cellSet));
  }

  VTKM_CONT_EXPORT
  vtkm::cont::DynamicCellSet GetCellSet(vtkm::Id index=0) const
  {
    VTKM_ASSERT_CONT((index >= 0) &&
                     (index < this->GetNumberOfCellSets()));
    return this->CellSets[static_cast<std::size_t>(index)];
  }

  VTKM_CONT_EXPORT
  vtkm::cont::DynamicCellSet GetCellSet(const std::string &name)
      const
  {
    for (std::size_t i=0; i < static_cast<size_t>(this->GetNumberOfCellSets()); ++i)
    {
      if (this->CellSets[i].GetCellSet().GetName() == name)
      {
        return this->CellSets[i];
      }
    }
    throw vtkm::cont::ErrorControlBadValue("No cell set with requested name");
  }

  VTKM_CONT_EXPORT
  vtkm::IdComponent GetNumberOfCellSets() const
  {
    return static_cast<vtkm::IdComponent>(this->CellSets.size());
  }

  VTKM_CONT_EXPORT
  vtkm::IdComponent GetNumberOfFields() const
  {
    return static_cast<vtkm::IdComponent>(this->Fields.size());
  }

  VTKM_CONT_EXPORT
  vtkm::IdComponent GetNumberOfCoordinateSystems() const
  {
    return static_cast<vtkm::IdComponent>(this->CoordSystems.size());
  }

  VTKM_CONT_EXPORT
  void PrintSummary(std::ostream &out) const
  {
      out<<"DataSet:\n";
      out<<"  CoordSystems["<<this->CoordSystems.size()<<"]\n";
      for (std::size_t i = 0; i < this->CoordSystems.size(); i++)
      {
        this->CoordSystems[i].PrintSummary(out);
      }

      out<<"  CellSets["<<this->GetNumberOfCellSets()<<"]\n";
      for (vtkm::Id i = 0; i < this->GetNumberOfCellSets(); i++)
      {
        this->GetCellSet(i).GetCellSet().PrintSummary(out);
      }

      out<<"  Fields["<<this->GetNumberOfFields()<<"]\n";
      for (vtkm::Id i = 0; i < this->GetNumberOfFields(); i++)
      {
        this->GetField(i).PrintSummary(out);
      }
  }

private:
  std::vector<vtkm::cont::CoordinateSystem> CoordSystems;
  std::vector<vtkm::cont::Field> Fields;
  std::vector<vtkm::cont::DynamicCellSet> CellSets;
};

} // namespace cont
} // namespace vtkm


#endif //vtk_m_cont_DataSet_h
