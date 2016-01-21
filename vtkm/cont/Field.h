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

#include <vtkm/Types.h>
#include <vtkm/Math.h>
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

template<vtkm::IdComponent NumberOfComponents>
class InputToOutputTypeTransform
{
public:
  typedef vtkm::Vec<vtkm::Float64, NumberOfComponents> ResultType;
  typedef vtkm::Pair<ResultType, ResultType> MinMaxPairType;

  template<typename ValueType>
  VTKM_EXEC_EXPORT
  MinMaxPairType operator()(const ValueType &value) const
  {
    ResultType input;
    for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      input[i] = static_cast<vtkm::Float64>(
          vtkm::VecTraits<ValueType>::GetComponent(value, i));
    }
    return make_Pair(input, input);
  }
};

template<vtkm::IdComponent NumberOfComponents>
class MinMax
{
public:
  typedef vtkm::Vec<vtkm::Float64, NumberOfComponents> ResultType;
  typedef vtkm::Pair<ResultType, ResultType> MinMaxPairType;

  VTKM_EXEC_EXPORT
  MinMaxPairType operator()(const MinMaxPairType &v1, const MinMaxPairType &v2) const
  {
    MinMaxPairType result;
    for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      result.first[i] = vtkm::Min(v1.first[i], v2.first[i]);
      result.second[i] = vtkm::Max(v1.second[i], v2.second[i]);
    }
    return result;
  }
};

enum
{
  MAX_NUMBER_OF_COMPONENTS = 10
};

template<vtkm::IdComponent NumberOfComponents, typename ComputeBoundsClass>
class SelectNumberOfComponents
{
public:
  template<typename TypeList, typename StorageList>
  static void Execute(
      vtkm::IdComponent components,
      const vtkm::cont::DynamicArrayHandleBase<TypeList,StorageList> &data,
      ArrayHandle<vtkm::Float64> &bounds)
  {
    if (components == NumberOfComponents)
    {
      ComputeBoundsClass::template CallBody<NumberOfComponents>(data, bounds);
    }
    else
    {
      SelectNumberOfComponents<NumberOfComponents+1,
                               ComputeBoundsClass>::Execute(components,
                                                            data,
                                                            bounds);
    }
  }
};

template<typename ComputeBoundsClass>
class SelectNumberOfComponents<MAX_NUMBER_OF_COMPONENTS, ComputeBoundsClass>
{
public:
  template<typename TypeList, typename StorageList>
  static void Execute(vtkm::IdComponent,
                      const vtkm::cont::DynamicArrayHandleBase<TypeList,StorageList> &,
                      ArrayHandle<vtkm::Float64>&)
  {
    throw vtkm::cont::ErrorControlInternal(
        "Number of components in array greater than expected maximum.");
  }
};


template<typename DeviceAdapterTag>
class ComputeBounds
{
private:
  template<vtkm::IdComponent NumberOfComponents>
  class Body
  {
  public:
    Body(ArrayHandle<vtkm::Float64> *bounds) : Bounds(bounds) {}

    template<typename ArrayHandleType>
    void operator()(const ArrayHandleType &data) const
    {
      typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag> Algorithm;
      typedef vtkm::Vec<vtkm::Float64, NumberOfComponents> ResultType;
      typedef vtkm::Pair<ResultType, ResultType> MinMaxPairType;

      MinMaxPairType initialValue = make_Pair(ResultType(vtkm::Infinity64()),
                                              ResultType(vtkm::NegativeInfinity64()));

      vtkm::cont::ArrayHandleTransform<MinMaxPairType, ArrayHandleType,
          InputToOutputTypeTransform<NumberOfComponents> > input(data);

      MinMaxPairType result = Algorithm::Reduce(input, initialValue,
                                                MinMax<NumberOfComponents>());

      this->Bounds->Allocate(NumberOfComponents * 2);
      for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
      {
        this->Bounds->GetPortalControl().Set(i * 2, result.first[i]);
        this->Bounds->GetPortalControl().Set(i * 2 + 1, result.second[i]);
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

      this->Bounds->Allocate(6);
      vtkm::cont::ArrayHandle<vtkm::Float64>::PortalControl outPortal =
          this->Bounds->GetPortalControl();
      outPortal.Set(0, minimum[0]);
      outPortal.Set(1, maximum[0]);
      outPortal.Set(2, minimum[1]);
      outPortal.Set(3, maximum[1]);
      outPortal.Set(4, minimum[2]);
      outPortal.Set(5, maximum[2]);
    }

  private:
    vtkm::cont::ArrayHandle<vtkm::Float64> *Bounds;
  };

public:
  template<vtkm::IdComponent NumberOfComponents,
           typename TypeList,
           typename StorageList>
  static void CallBody(
      const vtkm::cont::DynamicArrayHandleBase<TypeList, StorageList> &data,
      ArrayHandle<vtkm::Float64> &bounds)
  {
    Body<NumberOfComponents> cb(&bounds);
    data.CastAndCall(cb);
  }

  template<typename TypeList, typename StorageList>
  static void DoCompute(
      const DynamicArrayHandleBase<TypeList,StorageList> &data,
      ArrayHandle<vtkm::Float64> &bounds)
  {
    typedef ComputeBounds<DeviceAdapterTag> SelfType;
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

    vtkm::IdComponent numberOfComponents = data.GetNumberOfComponents();
    switch(numberOfComponents)
      {
      case 1:
        CallBody<1>(data, bounds);
        break;
      case 2:
        CallBody<2>(data, bounds);
        break;
      case 3:
        CallBody<3>(data, bounds);
        break;
      case 4:
        CallBody<4>(data, bounds);
        break;
      default:
        SelectNumberOfComponents<5, SelfType>::Execute(numberOfComponents,
                                                       data,
                                                       bounds);
        break;
      }
  }
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
  VTKM_CONT_EXPORT
  Field(std::string name,
        vtkm::IdComponent order,
        AssociationEnum association,
        const vtkm::cont::DynamicArrayHandle &data)
    : Name(name),
      Order(order),
      Association(association),
      AssocCellSetName(),
      AssocLogicalDim(-1),
      Data(data),
      Bounds(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT_CONT(this->Association == ASSOC_WHOLE_MESH ||
                     this->Association == ASSOC_POINTS);
  }

  template<typename T, typename Storage>
  VTKM_CONT_EXPORT
  Field(std::string name,
        vtkm::IdComponent order,
        AssociationEnum association,
        const ArrayHandle<T, Storage> &data)
    : Name(name),
      Order(order),
      Association(association),
      AssocCellSetName(),
      AssocLogicalDim(-1),
      Data(data),
      Bounds(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT_CONT((this->Association == ASSOC_WHOLE_MESH) ||
                     (this->Association == ASSOC_POINTS));
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name,
        vtkm::IdComponent order,
        AssociationEnum association,
        const std::vector<T> &data)
    : Name(name),
      Order(order),
      Association(association),
      AssocCellSetName(),
      AssocLogicalDim(-1),
      Bounds(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT_CONT((this->Association == ASSOC_WHOLE_MESH) ||
                     (this->Association == ASSOC_POINTS));
    this->CopyData(&data[0], static_cast<vtkm::Id>(data.size()));
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name,
        vtkm::IdComponent order,
        AssociationEnum association,
        const T *data,
        vtkm::Id nvals)
    : Name(name),
      Order(order),
      Association(association),
      AssocCellSetName(),
      AssocLogicalDim(-1),
      Bounds(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT_CONT((this->Association == ASSOC_WHOLE_MESH) ||
                     (this->Association == ASSOC_POINTS));
    this->CopyData(data, nvals);
  }

  /// constructors for cell set associations
  VTKM_CONT_EXPORT
  Field(std::string name,
        vtkm::IdComponent order,
        AssociationEnum association,
        const std::string& cellSetName,
        const vtkm::cont::DynamicArrayHandle &data)
    : Name(name),
      Order(order),
      Association(association),
      AssocCellSetName(cellSetName),
      AssocLogicalDim(-1),
      Data(data),
      Bounds(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT_CONT(this->Association == ASSOC_CELL_SET);
  }

  template <typename T, typename Storage>
  VTKM_CONT_EXPORT
  Field(std::string name,
        vtkm::IdComponent order,
        AssociationEnum association,
        const std::string& cellSetName,
        const vtkm::cont::ArrayHandle<T, Storage> &data)
    : Name(name),
      Order(order),
      Association(association),
      AssocCellSetName(cellSetName),
      AssocLogicalDim(-1),
      Data(data),
      Bounds(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT_CONT(this->Association == ASSOC_CELL_SET);
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name,
        vtkm::IdComponent order,
        AssociationEnum association,
        const std::string& cellSetName,
        const std::vector<T> &data)
    : Name(name),
      Order(order),
      Association(association),
      AssocCellSetName(cellSetName),
      AssocLogicalDim(-1),
      Bounds(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT_CONT(this->Association == ASSOC_CELL_SET);
    this->CopyData(&data[0], static_cast<vtkm::Id>(data.size()));
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name,
        vtkm::IdComponent order,
        AssociationEnum association,
        const std::string& cellSetName,
        const T *data,
        vtkm::Id nvals)
    : Name(name),
      Order(order),
      Association(association),
      AssocCellSetName(cellSetName),
      AssocLogicalDim(-1),
      Bounds(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT_CONT(this->Association == ASSOC_CELL_SET);
    this->CopyData(data, nvals);
  }

  /// constructors for logical dimension associations
  VTKM_CONT_EXPORT
  Field(std::string name,
        vtkm::IdComponent order,
        AssociationEnum association,
        vtkm::IdComponent logicalDim,
        const vtkm::cont::DynamicArrayHandle &data)
    : Name(name),
      Order(order),
      Association(association),
      AssocCellSetName(),
      AssocLogicalDim(logicalDim),
      Data(data),
      Bounds(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT_CONT(this->Association == ASSOC_LOGICAL_DIM);
  }

  template <typename T, typename Storage>
  VTKM_CONT_EXPORT
  Field(std::string name,
        vtkm::IdComponent order,
        AssociationEnum association,
        vtkm::IdComponent logicalDim,
        const vtkm::cont::ArrayHandle<T, Storage> &data)
    : Name(name),
      Order(order),
      Association(association),
      AssocLogicalDim(logicalDim),
      Data(data),
      Bounds(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT_CONT(this->Association == ASSOC_LOGICAL_DIM);
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name,
        vtkm::IdComponent order,
        AssociationEnum association,
        vtkm::IdComponent logicalDim,
        const std::vector<T> &data)
    : Name(name),
      Order(order),
      Association(association),
      AssocLogicalDim(logicalDim),
      Bounds(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT_CONT(this->Association == ASSOC_LOGICAL_DIM);
    this->CopyData(&data[0], static_cast<vtkm::Id>(data.size()));
  }

  template <typename T>
  VTKM_CONT_EXPORT
  Field(std::string name,
        vtkm::IdComponent order,
        AssociationEnum association,
        vtkm::IdComponent logicalDim,
        const T *data, vtkm::Id nvals)
    : Name(name),
      Order(order),
      Association(association),
      AssocLogicalDim(logicalDim),
      Bounds(),
      ModifiedFlag(true)
  {
    VTKM_ASSERT_CONT(this->Association == ASSOC_LOGICAL_DIM);
    CopyData(data, nvals);
  }

  VTKM_CONT_EXPORT
  Field()
    : Name(),
      Order(),
      Association(),
      AssocCellSetName(),
      AssocLogicalDim(),
      Data(),
      Bounds(),
      ModifiedFlag(true)
  {
    //Generate an empty field
  }

  VTKM_CONT_EXPORT
  const std::string &GetName() const
  {
    return this->Name;
  }

  VTKM_CONT_EXPORT
  AssociationEnum GetAssociation() const
  {
    return this->Association;
  }

  VTKM_CONT_EXPORT
  vtkm::IdComponent GetOrder() const
  {
    return this->Order;
  }

  VTKM_CONT_EXPORT
  std::string GetAssocCellSet() const
  {
    return this->AssocCellSetName;
  }

  VTKM_CONT_EXPORT
  vtkm::IdComponent GetAssocLogicalDim() const
  {
    return this->AssocLogicalDim;
  }

  template<typename DeviceAdapterTag, typename TypeList, typename StorageList>
  VTKM_CONT_EXPORT
  const vtkm::cont::ArrayHandle<vtkm::Float64>& GetBounds(DeviceAdapterTag,
                                                          TypeList,
                                                          StorageList) const
  {
    if (this->ModifiedFlag)
    {
      internal::ComputeBounds<DeviceAdapterTag>::DoCompute(
          this->Data.ResetTypeList(TypeList()).ResetStorageList(StorageList()),
          this->Bounds);
      this->ModifiedFlag = false;
    }

    return this->Bounds;
  }

  template<typename DeviceAdapterTag, typename TypeList, typename StorageList>
  VTKM_CONT_EXPORT
  void GetBounds(vtkm::Float64 *bounds,
                 DeviceAdapterTag,
                 TypeList,
                 StorageList) const
  {
    this->GetBounds(DeviceAdapterTag(), TypeList(), StorageList());

    vtkm::Id length = this->Bounds.GetNumberOfValues();
    for (vtkm::Id i = 0; i < length; ++i)
    {
      bounds[i] = this->Bounds.GetPortalConstControl().Get(i);
    }
  }

  template<typename DeviceAdapterTag, typename TypeList>
  VTKM_CONT_EXPORT
  const vtkm::cont::ArrayHandle<vtkm::Float64>& GetBounds(DeviceAdapterTag,
                                                          TypeList) const
  {
    return this->GetBounds(DeviceAdapterTag(), TypeList(),
                           VTKM_DEFAULT_STORAGE_LIST_TAG());
  }

  template<typename DeviceAdapterTag, typename TypeList>
  VTKM_CONT_EXPORT
  void GetBounds(vtkm::Float64 *bounds, DeviceAdapterTag, TypeList) const
  {
    this->GetBounds(bounds, DeviceAdapterTag(), TypeList(),
                    VTKM_DEFAULT_STORAGE_LIST_TAG());
  }

  template<typename DeviceAdapterTag>
  VTKM_CONT_EXPORT
  const vtkm::cont::ArrayHandle<vtkm::Float64>& GetBounds(DeviceAdapterTag) const
  {
    return this->GetBounds(DeviceAdapterTag(), VTKM_DEFAULT_TYPE_LIST_TAG(),
                           VTKM_DEFAULT_STORAGE_LIST_TAG());
  }

  template<typename DeviceAdapterTag>
  VTKM_CONT_EXPORT
  void GetBounds(vtkm::Float64 *bounds, DeviceAdapterTag) const
  {
    this->GetBounds(bounds, DeviceAdapterTag(), VTKM_DEFAULT_TYPE_LIST_TAG(),
                    VTKM_DEFAULT_STORAGE_LIST_TAG());
  }

  VTKM_CONT_EXPORT
  const vtkm::cont::DynamicArrayHandle &GetData() const
  {
    return this->Data;
  }

  VTKM_CONT_EXPORT
  vtkm::cont::DynamicArrayHandle &GetData()
  {
    this->ModifiedFlag = true;
    return this->Data;
  }

  template <typename T>
  VTKM_CONT_EXPORT
  void SetData(const vtkm::cont::ArrayHandle<T> &newdata)
  {
    this->Data = newdata;
    this->ModifiedFlag = true;
  }

  VTKM_CONT_EXPORT
  void SetData(const vtkm::cont::DynamicArrayHandle &newdata)
  {
    this->Data = newdata;
    this->ModifiedFlag = true;
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
    this->ModifiedFlag = true;
  }

  VTKM_CONT_EXPORT
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
      //out<<" Order= "<<Order;
      out<<"\n";
  }

private:
  std::string       Name;  ///< name of field

  vtkm::IdComponent Order; ///< 0=(piecewise) constant, 1=linear, 2=quadratic
  AssociationEnum   Association;
  std::string       AssocCellSetName;  ///< only populate if assoc is cells
  vtkm::IdComponent AssocLogicalDim; ///< only populate if assoc is logical dim

  vtkm::cont::DynamicArrayHandle Data;
  mutable vtkm::cont::ArrayHandle<vtkm::Float64> Bounds;
  mutable bool ModifiedFlag;
};


} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_Field_h
