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

#include <vtkm/Math.h>
#include <vtkm/Range.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicArrayHandle.h>

#include <vtkm/cont/internal/ArrayPortalFromIterators.h>

namespace vtkm {
namespace cont {

namespace internal {

template <typename T>
struct MinMaxValue
{
  VTKM_EXEC_CONT
  vtkm::Pair<T, T> operator()(const T& a, const T& b) const
  {
    return vtkm::make_Pair(vtkm::Min(a, b), vtkm::Max(a, b));
  }

  VTKM_EXEC_CONT
  vtkm::Pair<T, T> operator()(
    const vtkm::Pair<T, T>& a, const vtkm::Pair<T, T>& b) const
  {
    return vtkm::make_Pair(
      vtkm::Min(a.first, b.first), vtkm::Max(a.second, b.second));
  }

  VTKM_EXEC_CONT
  vtkm::Pair<T, T> operator()(const T& a, const vtkm::Pair<T, T>& b) const
  {
    return vtkm::make_Pair(vtkm::Min(a, b.first), vtkm::Max(a, b.second));
  }

  VTKM_EXEC_CONT
  vtkm::Pair<T, T> operator()(const vtkm::Pair<T, T>& a, const T& b) const
  {
    return vtkm::make_Pair(vtkm::Min(a.first, b), vtkm::Max(a.second, b));
  }
};

template<typename DeviceAdapterTag>
class ComputeRange
{
public:
  ComputeRange(ArrayHandle<vtkm::Range>& range) : Range(&range) {}

  template<typename ArrayHandleType>
  void operator()(const ArrayHandleType &input) const
  {
    typedef typename ArrayHandleType::ValueType ValueType;
    typedef vtkm::VecTraits<ValueType> VecType;
    const vtkm::IdComponent NumberOfComponents = VecType::NUM_COMPONENTS;

    typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag> Algorithm;

    //not the greatest way of doing this for performance reasons. But
    //this implementation should generate the smallest amount of code
    const vtkm::Pair<ValueType, ValueType> initial(
      input.GetPortalConstControl().Get(0),
      input.GetPortalConstControl().Get(0));

    vtkm::Pair<ValueType, ValueType> result =
      Algorithm::Reduce(input, initial, MinMaxValue<ValueType>());

    this->Range->Allocate(NumberOfComponents);
    for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      this->Range->GetPortalControl().Set(
            i, vtkm::Range(VecType::GetComponent(result.first, i),
                           VecType::GetComponent(result.second, i)));
    }
  }

  // Special implementation for regular point coordinates, which are easy
  // to determine.
  void operator()(const vtkm::cont::ArrayHandle<
                      vtkm::Vec<vtkm::FloatDefault,3>,
                      vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag>
                    &array)
  {
    vtkm::internal::ArrayPortalUniformPointCoordinates portal =
        array.GetPortalConstControl();

    // In this portal we know that the min value is the first entry and the
    // max value is the last entry.
    vtkm::Vec<vtkm::FloatDefault,3> minimum = portal.Get(0);
    vtkm::Vec<vtkm::FloatDefault,3> maximum =
        portal.Get(portal.GetNumberOfValues()-1);

    this->Range->Allocate(3);
    vtkm::cont::ArrayHandle<vtkm::Range>::PortalControl outPortal =
        this->Range->GetPortalControl();
    outPortal.Set(0, vtkm::Range(minimum[0], maximum[0]));
    outPortal.Set(1, vtkm::Range(minimum[1], maximum[1]));
    outPortal.Set(2, vtkm::Range(minimum[2], maximum[2]));
  }

private:
    vtkm::cont::ArrayHandle<vtkm::Range> *Range;
};

} // namespace internal


/// A \c Field encapsulates an array on some piece of the mesh, such as
/// the points, a cell set, a point logical dimension, or the whole mesh.
///
class Field
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
  Field(std::string name,
        AssociationEnum association,
        const vtkm::cont::DynamicArrayHandle &data)
    : Name(name),
      Association(association),
      AssocCellSetName(),
      AssocLogicalDim(-1),
      Data(data),
      Range(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_WHOLE_MESH ||
                this->Association == ASSOC_POINTS);
  }

  template<typename T, typename Storage>
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        const ArrayHandle<T, Storage> &data)
    : Name(name),
      Association(association),
      AssocCellSetName(),
      AssocLogicalDim(-1),
      Data(data),
      Range(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT((this->Association == ASSOC_WHOLE_MESH) ||
                (this->Association == ASSOC_POINTS));
  }

  template <typename T>
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        const std::vector<T> &data)
    : Name(name),
      Association(association),
      AssocCellSetName(),
      AssocLogicalDim(-1),
      Range(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT((this->Association == ASSOC_WHOLE_MESH) ||
                (this->Association == ASSOC_POINTS));
    this->CopyData(&data[0], static_cast<vtkm::Id>(data.size()));
  }

  template <typename T>
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        const T *data,
        vtkm::Id nvals)
    : Name(name),
      Association(association),
      AssocCellSetName(),
      AssocLogicalDim(-1),
      Range(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT((this->Association == ASSOC_WHOLE_MESH) ||
                (this->Association == ASSOC_POINTS));
    this->CopyData(data, nvals);
  }

  /// constructors for cell set associations
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        const std::string& cellSetName,
        const vtkm::cont::DynamicArrayHandle &data)
    : Name(name),
      Association(association),
      AssocCellSetName(cellSetName),
      AssocLogicalDim(-1),
      Data(data),
      Range(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_CELL_SET);
  }

  template <typename T, typename Storage>
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        const std::string& cellSetName,
        const vtkm::cont::ArrayHandle<T, Storage> &data)
    : Name(name),
      Association(association),
      AssocCellSetName(cellSetName),
      AssocLogicalDim(-1),
      Data(data),
      Range(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_CELL_SET);
  }

  template <typename T>
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        const std::string& cellSetName,
        const std::vector<T> &data)
    : Name(name),
      Association(association),
      AssocCellSetName(cellSetName),
      AssocLogicalDim(-1),
      Range(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_CELL_SET);
    this->CopyData(&data[0], static_cast<vtkm::Id>(data.size()));
  }

  template <typename T>
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        const std::string& cellSetName,
        const T *data,
        vtkm::Id nvals)
    : Name(name),
      Association(association),
      AssocCellSetName(cellSetName),
      AssocLogicalDim(-1),
      Range(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_CELL_SET);
    this->CopyData(data, nvals);
  }

  /// constructors for logical dimension associations
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        vtkm::IdComponent logicalDim,
        const vtkm::cont::DynamicArrayHandle &data)
    : Name(name),
      Association(association),
      AssocCellSetName(),
      AssocLogicalDim(logicalDim),
      Data(data),
      Range(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_LOGICAL_DIM);
  }

  template <typename T, typename Storage>
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        vtkm::IdComponent logicalDim,
        const vtkm::cont::ArrayHandle<T, Storage> &data)
    : Name(name),
      Association(association),
      AssocLogicalDim(logicalDim),
      Data(data),
      Range(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_LOGICAL_DIM);
  }

  template <typename T>
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        vtkm::IdComponent logicalDim,
        const std::vector<T> &data)
    : Name(name),
      Association(association),
      AssocLogicalDim(logicalDim),
      Range(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_LOGICAL_DIM);
    this->CopyData(&data[0], static_cast<vtkm::Id>(data.size()));
  }

  template <typename T>
  VTKM_CONT
  Field(std::string name,
        AssociationEnum association,
        vtkm::IdComponent logicalDim,
        const T *data, vtkm::Id nvals)
    : Name(name),
      Association(association),
      AssocLogicalDim(logicalDim),
      Range(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT(this->Association == ASSOC_LOGICAL_DIM);
    CopyData(data, nvals);
  }

  VTKM_CONT
  Field()
    : Name(),
      Association(ASSOC_ANY),
      AssocCellSetName(),
      AssocLogicalDim(),
      Data(),
      Range(),
      ModifiedFlag(true)
  {
    //Generate an empty field
  }

  VTKM_CONT
  const std::string &GetName() const
  {
    return this->Name;
  }

  VTKM_CONT
  AssociationEnum GetAssociation() const
  {
    return this->Association;
  }

  VTKM_CONT
  std::string GetAssocCellSet() const
  {
    return this->AssocCellSetName;
  }

  VTKM_CONT
  vtkm::IdComponent GetAssocLogicalDim() const
  {
    return this->AssocLogicalDim;
  }

  template<typename DeviceAdapterTag, typename TypeList, typename StorageList>
  VTKM_CONT
  const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange(DeviceAdapterTag,
                                                       TypeList,
                                                       StorageList) const
  {
    if (this->ModifiedFlag)
    {
      internal::ComputeRange<DeviceAdapterTag> computeRange(this->Range);
      this->Data.ResetTypeAndStorageLists(TypeList(),StorageList()).CastAndCall(computeRange);
      this->ModifiedFlag = false;
    }

    return this->Range;
  }

  template<typename DeviceAdapterTag, typename TypeList, typename StorageList>
  VTKM_CONT
  void GetRange(vtkm::Range *range,
                DeviceAdapterTag,
                TypeList,
                StorageList) const
  {
    this->GetRange(DeviceAdapterTag(), TypeList(), StorageList());

    vtkm::Id length = this->Range.GetNumberOfValues();
    for (vtkm::Id i = 0; i < length; ++i)
    {
      range[i] = this->Range.GetPortalConstControl().Get(i);
    }
  }

  template<typename DeviceAdapterTag, typename TypeList>
  VTKM_CONT
  const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange(DeviceAdapterTag,
                                                       TypeList) const
  {
    return this->GetRange(DeviceAdapterTag(),
                          TypeList(),
                          VTKM_DEFAULT_STORAGE_LIST_TAG());
  }

  template<typename DeviceAdapterTag, typename TypeList>
  VTKM_CONT
  void GetRange(vtkm::Range *range, DeviceAdapterTag, TypeList) const
  {
    this->GetRange(range,
                   DeviceAdapterTag(),
                   TypeList(),
                   VTKM_DEFAULT_STORAGE_LIST_TAG());
  }

  template<typename DeviceAdapterTag>
  VTKM_CONT
  const vtkm::cont::ArrayHandle<vtkm::Range>& GetRange(DeviceAdapterTag) const
  {
    return this->GetRange(DeviceAdapterTag(),
                          VTKM_DEFAULT_TYPE_LIST_TAG(),
                          VTKM_DEFAULT_STORAGE_LIST_TAG());
  }

  template<typename DeviceAdapterTag>
  VTKM_CONT
  void GetRange(vtkm::Range *range, DeviceAdapterTag) const
  {
    this->GetRange(range,
                   DeviceAdapterTag(),
                   VTKM_DEFAULT_TYPE_LIST_TAG(),
                   VTKM_DEFAULT_STORAGE_LIST_TAG());
  }

  VTKM_CONT
  const vtkm::cont::DynamicArrayHandle &GetData() const
  {
    return this->Data;
  }

  VTKM_CONT
  vtkm::cont::DynamicArrayHandle &GetData()
  {
    this->ModifiedFlag = true;
    return this->Data;
  }

  template <typename T>
  VTKM_CONT
  void SetData(const vtkm::cont::ArrayHandle<T> &newdata)
  {
    this->Data = newdata;
    this->ModifiedFlag = true;
  }

  VTKM_CONT
  void SetData(const vtkm::cont::DynamicArrayHandle &newdata)
  {
    this->Data = newdata;
    this->ModifiedFlag = true;
  }

  template <typename T>
  VTKM_CONT
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
    this->ModifiedFlag = true;
  }

  VTKM_CONT
  virtual void PrintSummary(std::ostream &out) const
  {
      out<<"   "<<this->Name;
      out<<" assoc= ";
      switch (this->GetAssociation())
      {
      case ASSOC_ANY: out<<"Any "; break;
      case ASSOC_WHOLE_MESH: out<<"Mesh "; break;
      case ASSOC_POINTS: out<<"Points "; break;
      case ASSOC_CELL_SET: out<<"Cells "; break;
      case ASSOC_LOGICAL_DIM: out<<"LogicalDim "; break;
      }
      this->Data.PrintSummary(out);
      out<<"\n";
  }

private:
  std::string       Name;  ///< name of field

  AssociationEnum   Association;
  std::string       AssocCellSetName;  ///< only populate if assoc is cells
  vtkm::IdComponent AssocLogicalDim; ///< only populate if assoc is logical dim

  vtkm::cont::DynamicArrayHandle Data;
  mutable vtkm::cont::ArrayHandle<vtkm::Range> Range;
  mutable bool ModifiedFlag;
};

template<typename Functor>
void CastAndCall(const vtkm::cont::Field& field, const Functor &f)
{
  field.GetData().CastAndCall(f);
}

namespace internal {
template<>
struct DynamicTransformTraits<vtkm::cont::Field>
{
  typedef vtkm::cont::internal::DynamicTransformTagCastAndCall DynamicTag;
};


} // namespace internal
} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_Field_h
