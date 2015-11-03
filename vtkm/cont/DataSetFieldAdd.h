//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_cont_DataSetFieldAdd_h
#define vtk_m_cont_DataSetFieldAdd_h

namespace vtkm {
namespace cont {

class DataSetFieldAdd
{
public:
    VTKM_CONT_EXPORT
    DataSetFieldAdd() {}

    //Point centered fields.
    template <typename T, typename Storage>
    VTKM_CONT_EXPORT
    void AddPointField(vtkm::cont::DataSet &dataSet,
                       const std::string &fieldName,
                       vtkm::cont::ArrayHandle<T, Storage> &field)
    {
        dataSet.AddField(Field(fieldName, 1, vtkm::cont::Field::ASSOC_POINTS,
                               field));
    }

    template<typename T>
    VTKM_CONT_EXPORT
    void AddPointField(vtkm::cont::DataSet &dataSet,
                       const std::string &fieldName,
                       const std::vector<T> &field)
    {
        dataSet.AddField(Field(fieldName, 1, vtkm::cont::Field::ASSOC_POINTS,
                               field));
    }
    
    template<typename T>
    VTKM_CONT_EXPORT
    void AddPointField(vtkm::cont::DataSet &dataSet,
                       const std::string &fieldName,
                       const T *field, const vtkm::Id &n)
    {
        dataSet.AddField(Field(fieldName, 1, vtkm::cont::Field::ASSOC_POINTS,
                               field, n));
    }

    //Cell centered field
    template <typename T, typename Storage>
    VTKM_CONT_EXPORT
    void AddCellField(vtkm::cont::DataSet &dataSet,
                      const std::string &fieldName,
                      vtkm::cont::ArrayHandle<T, Storage> &field,
                      const std::string &cellSetName)
    {
        dataSet.AddField(Field(fieldName, 1, vtkm::cont::Field::ASSOC_CELL_SET,
                               cellSetName, field));
    }

    template<typename T>
    VTKM_CONT_EXPORT
    void AddCellField(vtkm::cont::DataSet &dataSet,
                      const std::string &fieldName,
                      const std::vector<T> &field,
                      const std::string &cellSetName)
    {
        dataSet.AddField(Field(fieldName, 1, vtkm::cont::Field::ASSOC_CELL_SET,
                               cellSetName, field));
    }
    
    template<typename T>
    VTKM_CONT_EXPORT
    void AddCellField(vtkm::cont::DataSet &dataSet,
                      const std::string &fieldName,
                      const T *field, const vtkm::Id &n,
                      const std::string &cellSetName)
    {
        dataSet.AddField(Field(fieldName, 1, vtkm::cont::Field::ASSOC_CELL_SET,
                               cellSetName, field, n));
    }
};

}
}//namespace vtkm::cont


#endif //vtk_m_cont_DataSetFieldAdd_h
