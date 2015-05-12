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
#include <vtkm/cont/RegularConnectivity.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetStructured.h>

namespace vtkm {
namespace cont {

class CellSet;

class DataSet
{
public:
  DataSet() {}

  template <typename T>
  void AddFieldViaCopy(T *ptr, int nvals)
  {
    vtkm::cont::ArrayHandle<T> tmp = vtkm::cont::make_ArrayHandle(ptr, nvals);
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> array;
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::
      Copy(tmp, array);
    Fields.resize(Fields.size()+1);
    Fields[Fields.size()-1].SetData(array);
  }
  vtkm::cont::Field &GetField(int index)
  {
    return Fields[index];
  }

  vtkm::Id x_idx, y_idx, z_idx;

  vtkm::cont::CellSet *GetCellSet(int index=0)
  {
    return CellSets[index];
  }

  void AddCellSet(vtkm::cont::CellSet *cs)
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

private:
  std::vector<vtkm::cont::Field> Fields;
  std::vector<vtkm::cont::CellSet*> CellSets;
};

} // namespace cont
} // namespace vtkm


#endif //vtk_m_cont_DataSet_h
