//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_DataSetFieldAdd_h
#define vtk_m_cont_DataSetFieldAdd_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace cont
{

class DataSetFieldAdd
{
public:
  VTKM_CONT
  DataSetFieldAdd() {}

  //Point centered fields.
  VTKM_CONT
  static void AddPointField(vtkm::cont::DataSet& dataSet,
                            const std::string& fieldName,
                            const vtkm::cont::DynamicArrayHandle& field)
  {
    dataSet.AddField(Field(fieldName, vtkm::cont::Field::Association::POINTS, field));
  }

  template <typename T, typename Storage>
  VTKM_CONT static void AddPointField(vtkm::cont::DataSet& dataSet,
                                      const std::string& fieldName,
                                      const vtkm::cont::ArrayHandle<T, Storage>& field)
  {
    dataSet.AddField(Field(fieldName, vtkm::cont::Field::Association::POINTS, field));
  }

  template <typename T>
  VTKM_CONT static void AddPointField(vtkm::cont::DataSet& dataSet,
                                      const std::string& fieldName,
                                      const std::vector<T>& field)
  {
    dataSet.AddField(
      make_Field(fieldName, vtkm::cont::Field::Association::POINTS, field, vtkm::CopyFlag::On));
  }

  template <typename T>
  VTKM_CONT static void AddPointField(vtkm::cont::DataSet& dataSet,
                                      const std::string& fieldName,
                                      const T* field,
                                      const vtkm::Id& n)
  {
    dataSet.AddField(
      make_Field(fieldName, vtkm::cont::Field::Association::POINTS, field, n, vtkm::CopyFlag::On));
  }

  //Cell centered field
  VTKM_CONT
  static void AddCellField(vtkm::cont::DataSet& dataSet,
                           const std::string& fieldName,
                           const vtkm::cont::DynamicArrayHandle& field,
                           const std::string& cellSetName)
  {
    dataSet.AddField(
      Field(fieldName, vtkm::cont::Field::Association::CELL_SET, cellSetName, field));
  }

  template <typename T, typename Storage>
  VTKM_CONT static void AddCellField(vtkm::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const vtkm::cont::ArrayHandle<T, Storage>& field,
                                     const std::string& cellSetName)
  {
    dataSet.AddField(
      Field(fieldName, vtkm::cont::Field::Association::CELL_SET, cellSetName, field));
  }

  template <typename T>
  VTKM_CONT static void AddCellField(vtkm::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const std::vector<T>& field,
                                     const std::string& cellSetName)
  {
    dataSet.AddField(make_Field(
      fieldName, vtkm::cont::Field::Association::CELL_SET, cellSetName, field, vtkm::CopyFlag::On));
  }

  template <typename T>
  VTKM_CONT static void AddCellField(vtkm::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const T* field,
                                     const vtkm::Id& n,
                                     const std::string& cellSetName)
  {
    dataSet.AddField(make_Field(fieldName,
                                vtkm::cont::Field::Association::CELL_SET,
                                cellSetName,
                                field,
                                n,
                                vtkm::CopyFlag::On));
  }

  VTKM_CONT
  static void AddCellField(vtkm::cont::DataSet& dataSet,
                           const std::string& fieldName,
                           const vtkm::cont::DynamicArrayHandle& field,
                           vtkm::Id cellSetIndex = 0)
  {
    std::string cellSetName = dataSet.GetCellSet(cellSetIndex).GetName();
    DataSetFieldAdd::AddCellField(dataSet, fieldName, field, cellSetName);
  }
  template <typename T, typename Storage>
  VTKM_CONT static void AddCellField(vtkm::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const vtkm::cont::ArrayHandle<T, Storage>& field,
                                     vtkm::Id cellSetIndex = 0)
  {
    std::string cellSetName = dataSet.GetCellSet(cellSetIndex).GetName();
    DataSetFieldAdd::AddCellField(dataSet, fieldName, field, cellSetName);
  }
  template <typename T>
  VTKM_CONT static void AddCellField(vtkm::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const std::vector<T>& field,
                                     vtkm::Id cellSetIndex = 0)
  {
    std::string cellSetName = dataSet.GetCellSet(cellSetIndex).GetName();
    DataSetFieldAdd::AddCellField(dataSet, fieldName, field, cellSetName);
  }

  template <typename T>
  VTKM_CONT static void AddCellField(vtkm::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const T* field,
                                     const vtkm::Id& n,
                                     vtkm::Id cellSetIndex = 0)
  {
    std::string cellSetName = dataSet.GetCellSet(cellSetIndex).GetName();
    DataSetFieldAdd::AddCellField(dataSet, fieldName, field, n, cellSetName);
  }
};
}
} //namespace vtkm::cont

#endif //vtk_m_cont_DataSetFieldAdd_h
