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
#ifndef vtk_m_cont_Field_h
#define vtk_m_cont_Field_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>

#include <vtkm/cont/internal/ArrayPortalFromIterators.h>

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
  VTKM_CONT_EXPORT
  Field(std::string n, int o, Association a, ArrayHandle<T> &d)
    : name(n), order(o), association(a)
  {
    VTKM_ASSERT_CONT(association == ASSOC_WHOLE_MESH ||
                     association == ASSOC_POINTS);
    SetData(d);
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string n, int o, Association a, const std::vector<T> &d)
    : name(n), order(o), association(a)
  {
    VTKM_ASSERT_CONT(association == ASSOC_WHOLE_MESH ||
                     association == ASSOC_POINTS);
    CopyData(&d[0], d.size());
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string n, int o, Association a, const T *d, vtkm::Id nvals)
    : name(n), order(o), association(a)
  {
    VTKM_ASSERT_CONT(association == ASSOC_WHOLE_MESH ||
                     association == ASSOC_POINTS);
    CopyData(d, nvals);
  }

  template<typename T>
  VTKM_CONT_EXPORT
  Field(std::string n, int o, Association a, T)
    : name(n), order(o), association(a), data(vtkm::cont::ArrayHandle<T>())
  {
    VTKM_ASSERT_CONT(association == ASSOC_WHOLE_MESH ||
                     association == ASSOC_POINTS);
  }

  /// constructors for cell set associations
  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string n, int o, Association a, const std::string& csn, ArrayHandle<T> &d)
    : name(n), order(o), association(a), assoc_cellset_name(csn)
  {
    VTKM_ASSERT_CONT(association == ASSOC_CELL_SET);
    SetData(d);
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string n, int o, Association a, const std::string& csn, const std::vector<T> &d)
    : name(n), order(o), association(a), assoc_cellset_name(csn)
  {
    VTKM_ASSERT_CONT(association == ASSOC_CELL_SET);
    CopyData(&d[0], d.size());
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string n, int o, Association a, const std::string& csn, const T *d, vtkm::Id nvals)
    : name(n), order(o), association(a), assoc_cellset_name(csn)
  {
    VTKM_ASSERT_CONT(association == ASSOC_CELL_SET);
    CopyData(d, nvals);
  }

  template<typename T>
  VTKM_CONT_EXPORT
  Field(std::string n, int o, Association a, const std::string& csn, T)
    : name(n), order(o), association(a), assoc_cellset_name(csn), data(vtkm::cont::ArrayHandle<T>())
  {
    VTKM_ASSERT_CONT(association == ASSOC_CELL_SET);
  }

  /// constructors for logical dimension associations
  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string n, int o, Association a, int l, ArrayHandle<T> &d)
    : name(n), order(o), association(a), assoc_logical_dim(l)
  {
    VTKM_ASSERT_CONT(association == ASSOC_LOGICAL_DIM);
    SetData(d);
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string n, int o, Association a, int l, const std::vector<T> &d)
    : name(n), order(o), association(a), assoc_logical_dim(l)
  {
    VTKM_ASSERT_CONT(association == ASSOC_LOGICAL_DIM);
    CopyData(&d[0], d.size());
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string n, int o, Association a, int l, const T *d, vtkm::Id nvals)
    : name(n), order(o), association(a), assoc_logical_dim(l)
  {
    VTKM_ASSERT_CONT(association == ASSOC_LOGICAL_DIM);
    CopyData(d, nvals);
  }

  template<typename T>
  VTKM_CONT_EXPORT
  Field(std::string n, int o, Association a, int l, T)
    : name(n), order(o), association(a), assoc_logical_dim(l), data(vtkm::cont::ArrayHandle<T>())
  {
    VTKM_ASSERT_CONT(association == ASSOC_LOGICAL_DIM);
  }

  VTKM_CONT_EXPORT
  const std::string &GetName()
  {
    return name;
  }

  VTKM_CONT_EXPORT
  Association GetAssociation()
  {
    return association;
  }

  VTKM_CONT_EXPORT
  int GetOrder()
  {
    return order;
  }

  VTKM_CONT_EXPORT
  std::string GetAssocCellSet()
  {
    return assoc_cellset_name;
  }

  VTKM_CONT_EXPORT
  int GetAssocLogicalDim()
  {
    return assoc_logical_dim;
  }

  VTKM_CONT_EXPORT
  vtkm::cont::DynamicArrayHandle &GetData()
  {
    return data;
  }

  template <typename T>
  VTKM_CONT_EXPORT
  void SetData(vtkm::cont::ArrayHandle<T> &newdata)
  {
    data = newdata;
  }

  template <typename T>
  VTKM_CONT_EXPORT
  void CopyData(const T *ptr, vtkm::Id nvals)
  {
    //allocate main memory using an array handle
    vtkm::cont::ArrayHandle<T> tmp;
    tmp.Allocate(nvals);

    //copy into the memory owned by the array handle
    std::copy(ptr,
              ptr + static_cast<std::size_t>(nvals),
              vtkm::cont::ArrayPortalToIteratorBegin(tmp.GetPortalControl()));

    //assign to the dynamic array handle
    data = tmp;
  }

  VTKM_CONT_EXPORT
  virtual void PrintSummary(std::ostream &out)
  {
      out<<"   "<<name;
      out<<" assoc= ";
      switch (GetAssociation())
      {
      case ASSOC_WHOLE_MESH: out<<"Mesh "; break;
      case ASSOC_POINTS: out<<"Points "; break;
      case ASSOC_CELL_SET: out<<"Cells "; break;
      case ASSOC_LOGICAL_DIM: out<<"LogicalDim "; break;
      }
      vtkm::cont::ArrayHandle<vtkm::Float32> vals;
      vals = data.CastToArrayHandle(vtkm::Float32(), VTKM_DEFAULT_STORAGE_TAG());
      printSummary_ArrayHandle(vals, out);
      //out<<" order= "<<order;
      out<<"\n";
  }

private:
  std::string  name;  ///< name of field

  int          order; ///< 0=(piecewise) constant, 1=linear, 2=quadratic
  Association  association;
  std::string  assoc_cellset_name;  ///< only populate if assoc is cells
  int          assoc_logical_dim; ///< only populate if assoc is logical dim

  vtkm::cont::DynamicArrayHandle data;
};


} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_Field_h
