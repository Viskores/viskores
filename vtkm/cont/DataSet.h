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

class CellSet;

class DataSet
{
public:
  DataSet()
  {
  }

  void AddField(Field f)
  {
    Fields.push_back(f);
  }

  vtkm::cont::Field &GetField(int index)
  {
    VTKM_ASSERT_CONT(index >= 0 && index <= int(Fields.size()));
    return Fields[index];
  }

  vtkm::cont::Field &GetField(const std::string &n)
  {
    for (unsigned int i=0; i<Fields.size(); ++i)
    {
      if (Fields[i].GetName() == n)
        return Fields[i];
    }
    throw vtkm::cont::ErrorControlBadValue("No field with requested name");
  }

  boost::shared_ptr<vtkm::cont::CellSet> GetCellSet(int index=0)
  {
    VTKM_ASSERT_CONT(index >= 0 && index <= int(CellSets.size()));
    return CellSets[index];
  }

  void AddCoordinateSystem(vtkm::cont::CoordinateSystem cs)
  {
    CoordSystems.push_back(cs);
  }

  void AddCellSet(boost::shared_ptr<vtkm::cont::CellSet> cs)
  {
    CellSets.push_back(cs);
  }

  vtkm::Id GetNumberOfCellSets()
  {
    return static_cast<vtkm::Id>(this->CellSets.size());
  }

  vtkm::Id GetNumberOfFields()
  {
    return static_cast<vtkm::Id>(this->Fields.size());
  }

  void PrintSummary(std::ostream &out)
  {
      out<<"DataSet:\n";
      out<<"  CoordSystems["<<CoordSystems.size()<<"]\n";
      for (vtkm::Id i = 0; i < CoordSystems.size(); i++)
	  CoordSystems[i].PrintSummary(out);
      out<<"  CellSets["<<GetNumberOfCellSets()<<"]\n";
      for (vtkm::Id i = 0; i < GetNumberOfCellSets(); i++)
      	  GetCellSet(i)->PrintSummary(out);
      out<<"  Fields["<<GetNumberOfFields()<<"]\n";
      for (vtkm::Id i = 0; i < GetNumberOfFields(); i++)
      	  GetField(i).PrintSummary(out);
  }

private:
  std::vector<vtkm::cont::CoordinateSystem> CoordSystems;
  std::vector<vtkm::cont::Field> Fields;
  std::vector< boost::shared_ptr<vtkm::cont::CellSet> > CellSets;
};

} // namespace cont
} // namespace vtkm


#endif //vtk_m_cont_DataSet_h
