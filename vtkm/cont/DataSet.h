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

#include <vtkm/CellType.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ExplicitConnectivity.h>
#include <vtkm/RegularConnectivity.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ErrorControlBadValue.h>

#include <boost/smart_ptr/shared_ptr.hpp>

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
  void AddField(Field field)
  {
    this->Fields.push_back(field);
  }

  VTKM_CONT_EXPORT
  const vtkm::cont::Field &GetField(vtkm::Id index) const
  {
    VTKM_ASSERT_CONT((index >= 0) &&
                     (index <= static_cast<vtkm::Id>(this->Fields.size())));
    return this->Fields[static_cast<std::size_t>(index)];
  }

  VTKM_CONT_EXPORT
  const vtkm::cont::Field &GetField(const std::string &name) const
  {
    for (std::size_t i=0; i < this->Fields.size(); ++i)
    {
      if (this->Fields[i].GetName() == name)
      {
        return this->Fields[i];
      }
    }
    throw vtkm::cont::ErrorControlBadValue("No field with requested name");
  }

  VTKM_CONT_EXPORT
  boost::shared_ptr<vtkm::cont::CellSet> GetCellSet(vtkm::Id index=0) const
  {
    VTKM_ASSERT_CONT((index >= 0) &&
                     (index <= static_cast<vtkm::Id>(this->CellSets.size())));
    return this->CellSets[static_cast<std::size_t>(index)];
  }

  VTKM_CONT_EXPORT
  void AddCoordinateSystem(vtkm::cont::CoordinateSystem cs)
  {
    this->CoordSystems.push_back(cs);
  }

  VTKM_CONT_EXPORT
  void AddCellSet(boost::shared_ptr<vtkm::cont::CellSet> cs)
  {
    this->CellSets.push_back(cs);
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfCellSets() const
  {
    return static_cast<vtkm::Id>(this->CellSets.size());
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfFields() const
  {
    return static_cast<vtkm::Id>(this->Fields.size());
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
        this->GetCellSet(i)->PrintSummary(out);
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
  std::vector< boost::shared_ptr<vtkm::cont::CellSet> > CellSets;
};

} // namespace cont
} // namespace vtkm


#endif //vtk_m_cont_DataSet_h
