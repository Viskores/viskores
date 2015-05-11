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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_Field_h
#define vtk_m_Field_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm {
namespace cont {

/// A \c Field encapsulates an array on some piece of the mesh, such as
/// the points, a cell set, a node logical dimension, or the whole mesh.
///
class Field
{
public:

    enum Association
    {
        ASSOC_WHOLE_MESH,
        ASSOC_POINTS,
        ASSOC_CELL_SET,
        ASSOC_LOGICAL_DIM
    };

  /// default constructor
  Field()
  {
  }

  /// constructor for points / whole mesh
  template <typename T>
  Field(int o, Association a, ArrayHandle<T> &d)
    : order(o), association(a), data(d)
  {
    VTKM_ASSERT_CONT(association == ASSOC_WHOLE_MESH ||
                     association == ASSOC_POINTS);
    SetData(d);
  }

  /// constructor for cell set associations
  template <typename T>
  Field(int o, Association a, std::string n, ArrayHandle<T> &d)
    : order(o), association(a), assoc_cellset_name(n), data(d)
  {
    VTKM_ASSERT_CONT(association == ASSOC_CELL_SET);
    SetData(d);
  }

  /// constructor for logical dimension associations
  template <typename T>
  Field(int o, Association a, int l, ArrayHandle<T> &d)
    : order(o), association(a), assoc_logical_dim(l), data(d)
  {
    VTKM_ASSERT_CONT(association == ASSOC_LOGICAL_DIM);
    SetData(d);
  }

  Association GetAssociation()
  {
    return association;
  }

  int GetOrder()
  {
    return order;
  }

  std::string GetAssocCellSet()
  {
    return assoc_cellset_name;
  }

  int GetAssocLogicalDim()
  {
    return assoc_logical_dim;
  }

  vtkm::cont::DynamicArrayHandle &GetData()
  {
    return data;
  }

  template <typename T>
  void SetData(vtkm::cont::ArrayHandle<T> &newdata)
  {
    data = newdata;
  }

  /*
  void CopyIntoData(vtkm::cont::ArrayHandle<vtkm::FloatDefault> &tmp)
  {
    data = vtkm::cont::ArrayHandle<vtkm::FloatDefault, vtkm::cont::StorageTagBasic>();
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::
      Copy(tmp, data);
  }
  */
  
private:
  int          order; ///< 0=(piecewise) constant, 1=linear, 2=quadratic
  Association  association;
  std::string  assoc_cellset_name;  ///< only populate if assoc is cells
  int          assoc_logical_dim; ///< only populate if assoc is logical dim

  //vtkm::cont::ArrayHandle<vtkm::FloatDefault> data;
  vtkm::cont::DynamicArrayHandle data;
};


} // namespace cont
} // namespace vtkm

#endif //vtk_m_CellType_h
