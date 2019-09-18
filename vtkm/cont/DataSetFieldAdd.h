//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
                            const vtkm::cont::VariantArrayHandle& field)
  {
    dataSet.AddField(make_FieldPoint(fieldName, field));
  }

  template <typename T, typename Storage>
  VTKM_CONT static void AddPointField(vtkm::cont::DataSet& dataSet,
                                      const std::string& fieldName,
                                      const vtkm::cont::ArrayHandle<T, Storage>& field)
  {
    dataSet.AddField(make_FieldPoint(fieldName, field));
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
                           const vtkm::cont::VariantArrayHandle& field)
  {
    dataSet.AddField(make_FieldCell(fieldName, field));
  }

  template <typename T, typename Storage>
  VTKM_CONT static void AddCellField(vtkm::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const vtkm::cont::ArrayHandle<T, Storage>& field)
  {
    dataSet.AddField(make_FieldCell(fieldName, field));
  }

  template <typename T>
  VTKM_CONT static void AddCellField(vtkm::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const std::vector<T>& field)
  {
    dataSet.AddField(
      make_Field(fieldName, vtkm::cont::Field::Association::CELL_SET, field, vtkm::CopyFlag::On));
  }

  template <typename T>
  VTKM_CONT static void AddCellField(vtkm::cont::DataSet& dataSet,
                                     const std::string& fieldName,
                                     const T* field,
                                     const vtkm::Id& n)
  {
    dataSet.AddField(make_Field(
      fieldName, vtkm::cont::Field::Association::CELL_SET, field, n, vtkm::CopyFlag::On));
  }
};
}
} //namespace vtkm::cont

#endif //vtk_m_cont_DataSetFieldAdd_h
