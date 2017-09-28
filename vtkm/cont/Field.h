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
#ifndef vtk_m_cont_Field_h
#define vtk_m_cont_Field_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Range.h>
#include <vtkm/Types.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ArrayRangeCompute.h>
#include <vtkm/cont/DynamicArrayHandle.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

class ComputeRange
{
public:
  ComputeRange(ArrayHandle<vtkm::Range>& range)
    : Range(&range)
  {
  }

  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType& input) const
  {
    *this->Range = vtkm::cont::ArrayRangeCompute(input);
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Range>* Range;
};

} // namespace internal

/// A \c Field encapsulates an array on some piece of the mesh, such as
/// the points, a cell set, a point logical dimension, or the whole mesh.
///
class VTKM_CONT_EXPORT Field
{
public:
  enum AssociationEnum
  {
    ASSOC_ANY,
    ASSOC_WHOLE_MESH,
    ASSOC_POINTS,
    ASSOC_CELL_SET,
    ASSOC_LOGICAL_DIM
  };

  /// constructors for points / whole mesh
  VTKM_CONT
  Field(std::string name, AssociationEnum association, const vtkm::cont::DynamicArrayHandle& data)
    : Name(name)
    , Association(association)
    , AssocCellSetName()
    , AssocLogicalDim(-1)
    , Data(data)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_WHOLE_MESH || this->Association == ASSOC_POINTS);
  }

  template <typename T, typename Storage>
  VTKM_CONT Field(std::string name,
                  AssociationEnum association,
                  const ArrayHandle<T, Storage>& data)
    : Name(name)
    , Association(association)
    , AssocCellSetName()
    , AssocLogicalDim(-1)
    , Data(data)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT((this->Association == ASSOC_WHOLE_MESH) || (this->Association == ASSOC_POINTS));
  }

  template <typename T>
  VTKM_CONT Field(std::string name, AssociationEnum association, const std::vector<T>& data)
    : Name(name)
    , Association(association)
    , AssocCellSetName()
    , AssocLogicalDim(-1)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT((this->Association == ASSOC_WHOLE_MESH) || (this->Association == ASSOC_POINTS));
    this->CopyData(&data[0], static_cast<vtkm::Id>(data.size()));
  }

  template <typename T>
  VTKM_CONT Field(std::string name, AssociationEnum association, const T* data, vtkm::Id nvals)
    : Name(name)
    , Association(association)
    , AssocCellSetName()
    , AssocLogicalDim(-1)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT((this->Association == ASSOC_WHOLE_MESH) || (this->Association == ASSOC_POINTS));
    this->CopyData(data, nvals);
  }

  /// constructors for cell set associations
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        const std::string& cellSetName,
        const vtkm::cont::DynamicArrayHandle& data)
    : Name(name)
    , Association(association)
    , AssocCellSetName(cellSetName)
    , AssocLogicalDim(-1)
    , Data(data)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_CELL_SET);
  }

  template <typename T, typename Storage>
  VTKM_CONT Field(std::string name,
                  AssociationEnum association,
                  const std::string& cellSetName,
                  const vtkm::cont::ArrayHandle<T, Storage>& data)
    : Name(name)
    , Association(association)
    , AssocCellSetName(cellSetName)
    , AssocLogicalDim(-1)
    , Data(data)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_CELL_SET);
  }

  template <typename T>
  VTKM_CONT Field(std::string name,
                  AssociationEnum association,
                  const std::string& cellSetName,
                  const std::vector<T>& data)
    : Name(name)
    , Association(association)
    , AssocCellSetName(cellSetName)
    , AssocLogicalDim(-1)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_CELL_SET);
    this->CopyData(&data[0], static_cast<vtkm::Id>(data.size()));
  }

  template <typename T>
  VTKM_CONT Field(std::string name,
                  AssociationEnum association,
                  const std::string& cellSetName,
                  const T* data,
                  vtkm::Id nvals)
    : Name(name)
    , Association(association)
    , AssocCellSetName(cellSetName)
    , AssocLogicalDim(-1)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_CELL_SET);
    this->CopyData(data, nvals);
  }

  /// constructors for logical dimension associations
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        vtkm::IdComponent logicalDim,
        const vtkm::cont::DynamicArrayHandle& data)
    : Name(name)
    , Association(association)
    , AssocCellSetName()
    , AssocLogicalDim(logicalDim)
    , Data(data)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_LOGICAL_DIM);
  }

  template <typename T, typename Storage>
  VTKM_CONT Field(std::string name,
                  AssociationEnum association,
                  vtkm::IdComponent logicalDim,
                  const vtkm::cont::ArrayHandle<T, Storage>& data)
    : Name(name)
    , Association(association)
    , AssocLogicalDim(logicalDim)
    , Data(data)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_LOGICAL_DIM);
  }

  template <typename T>
  VTKM_CONT Field(std::string name,
                  AssociationEnum association,
                  vtkm::IdComponent logicalDim,
                  const std::vector<T>& data)
    : Name(name)
    , Association(association)
    , AssocLogicalDim(logicalDim)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_LOGICAL_DIM);
    this->CopyData(&data[0], static_cast<vtkm::Id>(data.size()));
  }

  template <typename T>
  VTKM_CONT Field(std::string name,
                  AssociationEnum association,
                  vtkm::IdComponent logicalDim,
                  const T* data,
                  vtkm::Id nvals)
    : Name(name)
    , Association(association)
    , AssocLogicalDim(logicalDim)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_LOGICAL_DIM);
    CopyData(data, nvals);
  }

  VTKM_CONT
  Field()
    : Name()
    , Association(ASSOC_ANY)
    , AssocCellSetName()
    , AssocLogicalDim()
    , Data()
    , Range()
    , ModifiedFlag(true)
  {
    //Generate an empty field
  }

  VTKM_CONT
  Field& operator=(const vtkm::cont::Field& src) = default;

  VTKM_CONT
  const std::string& GetName() const { return this->Name; }

  VTKM_CONT
  AssociationEnum GetAssociation() const { return this->Association; }

  VTKM_CONT
  std::string GetAssocCellSet() const { return this->AssocCellSetName; }

  VTKM_CONT
  vtkm::IdComponent GetAssocLogicalDim() const { return this->AssocLogicalDim; }

  template <typename TypeList, typename StorageList>
  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange(TypeList, StorageList) const
  {
    VTKM_IS_LIST_TAG(TypeList);
    VTKM_IS_LIST_TAG(StorageList);

    return this->GetRangeImpl(TypeList(), StorageList());
  }

  VTKM_CONT
  const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange(VTKM_DEFAULT_TYPE_LIST_TAG,
                                                       VTKM_DEFAULT_STORAGE_LIST_TAG) const;

  template <typename TypeList, typename StorageList>
  VTKM_CONT void GetRange(vtkm::Range* range, TypeList, StorageList) const
  {
    VTKM_IS_LIST_TAG(TypeList);
    VTKM_IS_LIST_TAG(StorageList);

    this->GetRange(TypeList(), StorageList());

    vtkm::Id length = this->Range.GetNumberOfValues();
    for (vtkm::Id i = 0; i < length; ++i)
    {
      range[i] = this->Range.GetPortalConstControl().Get(i);
    }
  }

  template <typename TypeList>
  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange(TypeList) const
  {
    VTKM_IS_LIST_TAG(TypeList);

    return this->GetRange(TypeList(), VTKM_DEFAULT_STORAGE_LIST_TAG());
  }

  template <typename TypeList>
  VTKM_CONT void GetRange(vtkm::Range* range, TypeList) const
  {
    VTKM_IS_LIST_TAG(TypeList);

    this->GetRange(range, TypeList(), VTKM_DEFAULT_STORAGE_LIST_TAG());
  }

  VTKM_CONT
  const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange() const;

  VTKM_CONT
  void GetRange(vtkm::Range* range) const;

  const vtkm::cont::DynamicArrayHandle& GetData() const;

  vtkm::cont::DynamicArrayHandle& GetData();

  template <typename T>
  VTKM_CONT void SetData(const vtkm::cont::ArrayHandle<T>& newdata)
  {
    this->Data = newdata;
    this->ModifiedFlag = true;
  }

  VTKM_CONT
  void SetData(const vtkm::cont::DynamicArrayHandle& newdata)
  {
    this->Data = newdata;
    this->ModifiedFlag = true;
  }

  template <typename T>
  VTKM_CONT void CopyData(const T* ptr, vtkm::Id nvals)
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
    this->ModifiedFlag = true;
  }

  VTKM_CONT
  virtual void PrintSummary(std::ostream& out) const;

private:
  std::string Name; ///< name of field

  AssociationEnum Association;
  std::string AssocCellSetName;      ///< only populate if assoc is cells
  vtkm::IdComponent AssocLogicalDim; ///< only populate if assoc is logical dim

  vtkm::cont::DynamicArrayHandle Data;
  mutable vtkm::cont::ArrayHandle<vtkm::Range> Range;
  mutable bool ModifiedFlag;

  template <typename TypeList, typename StorageList>
  VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Range>& GetRangeImpl(TypeList, StorageList) const
  {
    VTKM_IS_LIST_TAG(TypeList);
    VTKM_IS_LIST_TAG(StorageList);

    if (this->ModifiedFlag)
    {
      internal::ComputeRange computeRange(this->Range);
      this->Data.ResetTypeAndStorageLists(TypeList(), StorageList()).CastAndCall(computeRange);
      this->ModifiedFlag = false;
    }

    return this->Range;
  }
};

template <typename Functor>
void CastAndCall(const vtkm::cont::Field& field, const Functor& f)
{
  field.GetData().CastAndCall(f);
}

namespace internal
{
template <>
struct DynamicTransformTraits<vtkm::cont::Field>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal
} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_Field_h
