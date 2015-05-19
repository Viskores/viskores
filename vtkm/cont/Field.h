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

  /// constructors for points / whole mesh
  template <typename T>
  Field(std::string n, int o, Association a, ArrayHandle<T> &d)
    : name(n), order(o), association(a)
  {
    VTKM_ASSERT_CONT(association == ASSOC_WHOLE_MESH ||
                     association == ASSOC_POINTS);
    SetData(d);
  }
  template <typename T>
  Field(std::string n, int o, Association a, const std::vector<T> &d)
    : name(n), order(o), association(a)
  {
    VTKM_ASSERT_CONT(association == ASSOC_WHOLE_MESH ||
                     association == ASSOC_POINTS);
    CopyData(&d[0], d.size());
  }
  template <typename T>
  Field(std::string n, int o, Association a, const T *d, int nvals)
    : name(n), order(o), association(a)
  {
    VTKM_ASSERT_CONT(association == ASSOC_WHOLE_MESH ||
                     association == ASSOC_POINTS);
    CopyData(d, nvals);
  }
  template <typename T>
  Field(std::string n, int o, Association a)
    : name(n), order(o), association(a)
  {
    VTKM_ASSERT_CONT(association == ASSOC_WHOLE_MESH ||
                     association == ASSOC_POINTS);
  }

  /// constructors for cell set associations
  template <typename T>
  Field(std::string n, int o, Association a, std::string csn, ArrayHandle<T> &d)
    : name(n), order(o), association(a), assoc_cellset_name(csn)
  {
    VTKM_ASSERT_CONT(association == ASSOC_CELL_SET);
    SetData(d);
  }
  template <typename T>
  Field(std::string n, int o, Association a, std::string csn, const std::vector<T> &d)
    : name(n), order(o), association(a), assoc_cellset_name(csn)
  {
    VTKM_ASSERT_CONT(association == ASSOC_CELL_SET);
    CopyData(&d[0], d.size());
  }
  template <typename T>
  Field(std::string n, int o, Association a, std::string csn, const T *d, int nvals)
    : name(n), order(o), association(a), assoc_cellset_name(csn)
  {
    VTKM_ASSERT_CONT(association == ASSOC_CELL_SET);
    CopyData(d, nvals);
  }
  template <typename T>
  Field(std::string n, int o, Association a, std::string csn)
    : name(n), order(o), association(a), assoc_cellset_name(csn)
  {
    VTKM_ASSERT_CONT(association == ASSOC_CELL_SET);
  }

  /// constructors for logical dimension associations
  template <typename T>
  Field(std::string n, int o, Association a, int l, ArrayHandle<T> &d)
    : name(n), order(o), association(a), assoc_logical_dim(l)
  {
    VTKM_ASSERT_CONT(association == ASSOC_LOGICAL_DIM);
    SetData(d);
  }
  template <typename T>
  Field(std::string n, int o, Association a, int l, const std::vector<T> &d)
    : name(n), order(o), association(a), assoc_logical_dim(l)
  {
    VTKM_ASSERT_CONT(association == ASSOC_LOGICAL_DIM);
    CopyData(&d[0], d.size());
  }
  template <typename T>
  Field(std::string n, int o, Association a, int l, const T *d, int nvals)
    : name(n), order(o), association(a), assoc_logical_dim(l)
  {
    VTKM_ASSERT_CONT(association == ASSOC_LOGICAL_DIM);
    CopyData(d, nvals);
  }
  template <typename T>
  Field(std::string n, int o, Association a, int l)
    : name(n), order(o), association(a), assoc_logical_dim(l)
  {
    VTKM_ASSERT_CONT(association == ASSOC_LOGICAL_DIM);
  }

  const std::string &GetName()
  {
    return name;
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

  template <typename T>
  void CopyData(const T *ptr, int nvals)
  {
    vtkm::cont::ArrayHandle<T> tmp1 = vtkm::cont::make_ArrayHandle(ptr, nvals);
    vtkm::cont::ArrayHandle<T> tmp2;
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::
      Copy(tmp1, tmp2);
    data = tmp2;
  }
  
private:
  std::string  name;  ///< name of field

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
