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
  enum struct Association
  {
    ANY,
    WHOLE_MESH,
    POINTS,
    CELL_SET,
    LOGICAL_DIM
  };

  /// constructors for points / whole mesh
  VTKM_CONT
  Field(std::string name, Association association, const vtkm::cont::DynamicArrayHandle& data)
    : Name(name)
    , FieldAssociation(association)
    , AssocCellSetName()
    , AssocLogicalDim(-1)
    , Data(data)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->FieldAssociation == Association::WHOLE_MESH ||
                this->FieldAssociation == Association::POINTS);
  }

  template <typename T, typename Storage>
  VTKM_CONT Field(std::string name, Association association, const ArrayHandle<T, Storage>& data)
    : Name(name)
    , FieldAssociation(association)
    , AssocCellSetName()
    , AssocLogicalDim(-1)
    , Data(data)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT((this->FieldAssociation == Association::WHOLE_MESH) ||
                (this->FieldAssociation == Association::POINTS));
  }

  /// constructors for cell set associations
  VTKM_CONT
  Field(std::string name,
        Association association,
        const std::string& cellSetName,
        const vtkm::cont::DynamicArrayHandle& data)
    : Name(name)
    , FieldAssociation(association)
    , AssocCellSetName(cellSetName)
    , AssocLogicalDim(-1)
    , Data(data)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->FieldAssociation == Association::CELL_SET);
  }

  template <typename T, typename Storage>
  VTKM_CONT Field(std::string name,
                  Association association,
                  const std::string& cellSetName,
                  const vtkm::cont::ArrayHandle<T, Storage>& data)
    : Name(name)
    , FieldAssociation(association)
    , AssocCellSetName(cellSetName)
    , AssocLogicalDim(-1)
    , Data(data)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->FieldAssociation == Association::CELL_SET);
  }

  /// constructors for logical dimension associations
  VTKM_CONT
  Field(std::string name,
        Association association,
        vtkm::IdComponent logicalDim,
        const vtkm::cont::DynamicArrayHandle& data)
    : Name(name)
    , FieldAssociation(association)
    , AssocCellSetName()
    , AssocLogicalDim(logicalDim)
    , Data(data)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->FieldAssociation == Association::LOGICAL_DIM);
  }

  template <typename T, typename Storage>
  VTKM_CONT Field(std::string name,
                  Association association,
                  vtkm::IdComponent logicalDim,
                  const vtkm::cont::ArrayHandle<T, Storage>& data)
    : Name(name)
    , FieldAssociation(association)
    , AssocLogicalDim(logicalDim)
    , Data(data)
    , Range()
    , ModifiedFlag(true)
  {
    VTKM_ASSERT(this->FieldAssociation == Association::LOGICAL_DIM);
  }

  VTKM_CONT
  Field()
    : Name()
    , FieldAssociation(Association::ANY)
    , AssocCellSetName()
    , AssocLogicalDim()
    , Data()
    , Range()
    , ModifiedFlag(true)
  {
    //Generate an empty field
  }

  VTKM_CONT
  virtual ~Field();

  VTKM_CONT
  Field& operator=(const vtkm::cont::Field& src) = default;

  VTKM_CONT
  const std::string& GetName() const { return this->Name; }

  VTKM_CONT
  Association GetAssociation() const { return this->FieldAssociation; }

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

  template <typename T, typename StorageTag>
  VTKM_CONT void SetData(const vtkm::cont::ArrayHandle<T, StorageTag>& newdata)
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
    this->Data = vtkm::cont::make_ArrayHandle(ptr, nvals, true);
    this->ModifiedFlag = true;
  }

  VTKM_CONT
  virtual void PrintSummary(std::ostream& out) const;

  VTKM_CONT
  virtual void ReleaseResourcesExecution()
  {
    // TODO: Call ReleaseResourcesExecution on the data when
    // the DynamicArrayHandle class is able to do so.
    this->Range.ReleaseResourcesExecution();
  }

private:
  std::string Name; ///< name of field

  Association FieldAssociation;
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

template <typename Functor, typename... Args>
void CastAndCall(const vtkm::cont::Field& field, Functor&& f, Args&&... args)
{
  field.GetData().CastAndCall(std::forward<Functor>(f), std::forward<Args>(args)...);
}

//@{
/// Convenience functions to build fields from C style arrays and std::vector
template <typename T>
vtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             const T* data,
                             vtkm::Id size,
                             vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  return vtkm::cont::Field(name, association, vtkm::cont::make_ArrayHandle(data, size, copy));
}

template <typename T>
vtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             const std::vector<T>& data,
                             vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  return vtkm::cont::Field(name, association, vtkm::cont::make_ArrayHandle(data, copy));
}

template <typename T>
vtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             const std::string& cellSetName,
                             const T* data,
                             vtkm::Id size,
                             vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  return vtkm::cont::Field(
    name, association, cellSetName, vtkm::cont::make_ArrayHandle(data, size, copy));
}

template <typename T>
vtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             const std::string& cellSetName,
                             const std::vector<T>& data,
                             vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  return vtkm::cont::Field(
    name, association, cellSetName, vtkm::cont::make_ArrayHandle(data, copy));
}

template <typename T>
vtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             vtkm::IdComponent logicalDim,
                             const T* data,
                             vtkm::Id size,
                             vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  return vtkm::cont::Field(
    name, association, logicalDim, vtkm::cont::make_ArrayHandle(data, size, copy));
}

template <typename T>
vtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             vtkm::IdComponent logicalDim,
                             const std::vector<T>& data,
                             vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  return vtkm::cont::Field(name, association, logicalDim, vtkm::cont::make_ArrayHandle(data, copy));
}
//@}

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
