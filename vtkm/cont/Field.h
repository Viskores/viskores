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
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association,
      vtkm::cont::DynamicArrayHandle &data)
    : Name(name), Order(order), AssocTag(association), Data(data)
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_WHOLE_MESH ||
                     this->AssocTag == ASSOC_POINTS);
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association, ArrayHandle<T> &data)
    : Name(name), Order(order), AssocTag(association)
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_WHOLE_MESH ||
                     this->AssocTag == ASSOC_POINTS);
    this->SetData(data);
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association,
      const std::vector<T> &data)
    : Name(name), Order(order), AssocTag(association)
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_WHOLE_MESH ||
                     this->AssocTag == ASSOC_POINTS);
    this->CopyData(&data[0], data.size());
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association, const T *data,
      vtkm::Id nvals)
    : Name(name), Order(order), AssocTag(association)
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_WHOLE_MESH ||
                     this->AssocTag == ASSOC_POINTS);
    this->CopyData(data, nvals);
  }

  template<typename T>
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association, T)
    : Name(name), Order(order), AssocTag(association),
      Data(vtkm::cont::ArrayHandle<T>())
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_WHOLE_MESH ||
                     this->AssocTag == ASSOC_POINTS);
  }

  /// constructors for cell set associations
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association,
      const std::string& cellSetName, vtkm::cont::DynamicArrayHandle &data)
    : Name(name), Order(order), AssocTag(association), AssocCellsetName(cellSetName),
      Data(data)
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_CELL_SET);
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association,
      const std::string& cellSetName, ArrayHandle<T> &data)
    : Name(name), Order(order), AssocTag(association), AssocCellsetName(cellSetName)
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_CELL_SET);
    this->SetData(data);
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association,
      const std::string& cellSetName, const std::vector<T> &data)
    : Name(name), Order(order), AssocTag(association), AssocCellsetName(cellSetName)
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_CELL_SET);
    this->CopyData(&data[0], data.size());
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association,
       const std::string& cellSetName, const T *data, vtkm::Id nvals)
    : Name(name), Order(order), AssocTag(association), AssocCellsetName(cellSetName)
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_CELL_SET);
    this->CopyData(data, nvals);
  }

  template<typename T>
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association,
      const std::string& cellSetName, T)
    : Name(name), Order(order), AssocTag(association), AssocCellsetName(cellSetName),
      Data(vtkm::cont::ArrayHandle<T>())
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_CELL_SET);
  }

  /// constructors for logical dimension associations
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association, int logicalDim,
      vtkm::cont::DynamicArrayHandle &data)
    : Name(name), Order(order), AssocTag(association), AssocLogicalDim(logicalDim),
      Data(data)
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_LOGICAL_DIM);
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association, int logicalDim,
      ArrayHandle<T> &data)
    : Name(name), Order(order), AssocTag(association), AssocLogicalDim(logicalDim)
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_LOGICAL_DIM);
    this->SetData(data);
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association, int logicalDim,
      const std::vector<T> &data)
    : Name(name), Order(order), AssocTag(association), AssocLogicalDim(logicalDim)
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_LOGICAL_DIM);
    this->CopyData(&data[0], data.size());
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association, int logicalDim,
      const T *data, vtkm::Id nvals)
    : Name(name), Order(order), AssocTag(association), AssocLogicalDim(logicalDim)
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_LOGICAL_DIM);
    this->CopyData(data, nvals);
  }

  template<typename T>
  VTKM_CONT_EXPORT
  Field(std::string name, int order, Association association, int logicalDim, T)
    : Name(name), Order(order), AssocTag(association), AssocLogicalDim(logicalDim),
      Data(vtkm::cont::ArrayHandle<T>())
  {
    VTKM_ASSERT_CONT(this->AssocTag == ASSOC_LOGICAL_DIM);
  }

  VTKM_CONT_EXPORT
  const std::string &GetName()
  {
    return this->Name;
  }

  VTKM_CONT_EXPORT
  Association GetAssociation()
  {
    return this->AssocTag;
  }

  VTKM_CONT_EXPORT
  int GetOrder()
  {
    return this->Order;
  }

  VTKM_CONT_EXPORT
  std::string GetAssocCellSet()
  {
    return this->AssocCellsetName;
  }

  VTKM_CONT_EXPORT
  int GetAssocLogicalDim()
  {
    return this->AssocLogicalDim;
  }

  VTKM_CONT_EXPORT
  vtkm::cont::DynamicArrayHandle &GetData()
  {
    return this->Data;
  }

  template <typename T>
  VTKM_CONT_EXPORT
  void SetData(vtkm::cont::ArrayHandle<T> &newdata)
  {
    this->Data = newdata;
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
    this->Data = tmp;
  }

  VTKM_CONT_EXPORT
  virtual void PrintSummary(std::ostream &out)
  {
      out<<"   "<<this->Name;
      out<<" assoc= ";
      switch (this->GetAssociation())
      {
      case ASSOC_WHOLE_MESH: out<<"Mesh "; break;
      case ASSOC_POINTS: out<<"Points "; break;
      case ASSOC_CELL_SET: out<<"Cells "; break;
      case ASSOC_LOGICAL_DIM: out<<"LogicalDim "; break;
      }
      vtkm::cont::ArrayHandle<vtkm::Float32> vals;
      vals = this->Data.CastToArrayHandle(vtkm::Float32(), VTKM_DEFAULT_STORAGE_TAG());
      printSummary_ArrayHandle(vals, out);
      //out<<" Order= "<<Order;
      out<<"\n";
  }

private:
  std::string  Name;  ///< Name of field

  int          Order; ///< 0=(piecewise) constant, 1=linear, 2=quadratic
  Association  AssocTag;
  std::string  AssocCellsetName;  ///< only populate if assoc is cells
  int          AssocLogicalDim; ///< only populate if assoc is logical dim

  vtkm::cont::DynamicArrayHandle Data;
};


} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_Field_h
