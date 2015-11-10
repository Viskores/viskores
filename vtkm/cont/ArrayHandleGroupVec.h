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
#ifndef vtk_m_cont_ArrayHandleGroupVec_h
#define vtk_m_cont_ArrayHandleGroupVec_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/Assert.h>
#include <vtkm/cont/ErrorControlBadValue.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/type_traits/remove_const.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace cont {

namespace internal {

template<typename _SourcePortalType, vtkm::IdComponent _NUM_COMPONENTS>
class ArrayPortalGroupVec
{
public:
  static const vtkm::IdComponent NUM_COMPONENTS = _NUM_COMPONENTS;
  typedef _SourcePortalType SourcePortalType;

  typedef typename
    boost::remove_const<typename SourcePortalType::ValueType>::type
      ComponentType;
  typedef vtkm::Vec<ComponentType, NUM_COMPONENTS> ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  ArrayPortalGroupVec() : SourcePortal() {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  ArrayPortalGroupVec(const SourcePortalType &sourcePortal)
    : SourcePortal(sourcePortal) {  }

  /// Copy constructor for any other ArrayPortalConcatenate with a portal type
  /// that can be copied to this portal type. This allows us to do any type
  /// casting that the portals do (like the non-const to const cast).
  VTKM_SUPPRESS_EXEC_WARNINGS
  template<typename OtherSourcePortalType>
  VTKM_EXEC_CONT_EXPORT
  ArrayPortalGroupVec(
      const ArrayPortalGroupVec<OtherSourcePortalType, NUM_COMPONENTS> &src)
    : SourcePortal(src.GetPortal()) {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    return this->SourcePortal.GetNumberOfValues()/NUM_COMPONENTS;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  ValueType Get(vtkm::Id index) const
  {
    ValueType result;
    vtkm::Id sourceIndex = index*NUM_COMPONENTS;
    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < NUM_COMPONENTS;
         componentIndex++)
    {
      result[componentIndex] = this->SourcePortal.Get(sourceIndex);
      sourceIndex++;
    }
    return result;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  void Set(vtkm::Id index, const ValueType &value) const
  {
    vtkm::Id sourceIndex = index*NUM_COMPONENTS;
    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < NUM_COMPONENTS;
         componentIndex++)
    {
      this->SourcePortal.Set(sourceIndex, value[componentIndex]);
      sourceIndex++;
    }
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  const SourcePortalType &GetPortal() const { this->SourcePortal; }

private:
  SourcePortalType SourcePortal;
};

template<typename SourceArrayHandleType, vtkm::IdComponent NUM_COMPONENTS>
struct StorageTagGroupVec {  };

template<typename SourceArrayHandleType,
         vtkm::IdComponent NUM_COMPONENTS>
class Storage<
    vtkm::Vec<typename SourceArrayHandleType::ValueType,NUM_COMPONENTS>,
    vtkm::cont::internal::StorageTagGroupVec<
      SourceArrayHandleType, NUM_COMPONENTS> >
{
  typedef typename SourceArrayHandleType::ValueType ComponentType;

public:
  typedef vtkm::Vec<ComponentType,NUM_COMPONENTS> ValueType;

  typedef vtkm::cont::internal::ArrayPortalGroupVec<
      typename SourceArrayHandleType::PortalControl,
      NUM_COMPONENTS> PortalType;
  typedef vtkm::cont::internal::ArrayPortalGroupVec<
      typename SourceArrayHandleType::PortalConstControl,
      NUM_COMPONENTS> PortalConstType;

  VTKM_CONT_EXPORT
  Storage() : Valid(false) {  }

  VTKM_CONT_EXPORT
  Storage(const SourceArrayHandleType &sourceArray)
    : SourceArray(sourceArray), Valid(true) {  }

  VTKM_CONT_EXPORT
  PortalType GetPortal()
  {
    VTKM_ASSERT_CONT(this->Valid);
    return PortalType(this->SourceArray.GetPortalControl());
  }

  VTKM_CONT_EXPORT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT_CONT(this->Valid);
    return PortalConstType(this->SourceArray.GetPortalConstControl());
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT_CONT(this->Valid);
    vtkm::Id sourceSize = this->SourceArray.GetNumberOfValues();
    if(sourceSize%NUM_COMPONENTS != 0)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "ArrayHandleGroupVec's source array does not divide evenly into Vecs.");
    }
    return sourceSize/NUM_COMPONENTS;
  }

  VTKM_CONT_EXPORT
  void Allocate(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT_CONT(this->Valid);
    this->SourceArray.Allocate(numberOfValues*NUM_COMPONENTS);
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT_CONT(this->Valid);
    this->SourceArray.Shrink(numberOfValues*NUM_COMPONENTS);
  }

  VTKM_CONT_EXPORT
  void ReleaseResources()
  {
    if (this->Valid)
    {
      this->SourceArray.ReleaseResources();
    }
  }

  // Required for later use in ArrayTransfer class
  VTKM_CONT_EXPORT
  const SourceArrayHandleType &GetSourceArray() const
  {
    VTKM_ASSERT_CONT(this->Valid);
    return this->SourceArray;
  }

private:
  SourceArrayHandleType SourceArray;
  bool Valid;
};

template<typename SourceArrayHandleType,
         vtkm::IdComponent NUM_COMPONENTS,
         typename Device>
class ArrayTransfer<
    vtkm::Vec<typename SourceArrayHandleType::ValueType, NUM_COMPONENTS>,
    vtkm::cont::internal::StorageTagGroupVec<
      SourceArrayHandleType, NUM_COMPONENTS>,
    Device>
{
public:
  typedef typename SourceArrayHandleType::ValueType ComponentType;
  typedef vtkm::Vec<ComponentType, NUM_COMPONENTS> ValueType;

private:
  typedef vtkm::cont::internal::StorageTagGroupVec<
    SourceArrayHandleType, NUM_COMPONENTS> StorageTag;
  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

public:
  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;

  typedef vtkm::cont::internal::ArrayPortalGroupVec<
      typename SourceArrayHandleType::template ExecutionTypes<Device>::Portal,
      NUM_COMPONENTS>
    PortalExecution;
  typedef vtkm::cont::internal::ArrayPortalGroupVec<
      typename SourceArrayHandleType::template ExecutionTypes<Device>::PortalConst,
      NUM_COMPONENTS>
    PortalConstExecution;

  VTKM_CONT_EXPORT
  ArrayTransfer(StorageType *storage)
    : SourceArray(storage->GetSourceArray()) {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    vtkm::Id sourceSize = this->SourceArray.GetNumberOfValues();
    if (sourceSize%NUM_COMPONENTS != 0)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "ArrayHandleGroupVec's source array does not divide evenly into Vecs.");
    }
    return sourceSize/NUM_COMPONENTS;
  }

  VTKM_CONT_EXPORT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    if (this->SourceArray.GetNumberOfValues()%NUM_COMPONENTS != 0)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "ArrayHandleGroupVec's source array does not divide evenly into Vecs.");
    }
    return PortalConstExecution(this->SourceArray.PrepareForInput(Device()));
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    if (this->SourceArray.GetNumberOfValues()%NUM_COMPONENTS != 0)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "ArrayHandleGroupVec's source array does not divide evenly into Vecs.");
    }
    return PortalExecution(this->SourceArray.PrepareForInPlace(Device()));
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return PortalExecution(this->SourceArray.PrepareForOutput(
                             numberOfValues*NUM_COMPONENTS, Device()));
  }

  VTKM_CONT_EXPORT
  void RetrieveOutputData(StorageType *vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handles should automatically retrieve the output data as
    // necessary.
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id numberOfValues)
  {
    this->SourceArray.Shrink(numberOfValues*NUM_COMPONENTS);
  }

  VTKM_CONT_EXPORT
  void ReleaseResources()
  {
    this->SourceArray.ReleaseResourcesExecution();
  }

private:
  SourceArrayHandleType SourceArray;
};

} // namespace internal

/// \brief Fancy array handle that groups values into vectors.
///
/// It is sometimes the case that an array is stored such that consecutive
/// entries are meant to form a group. This fancy array handle takes an array
/// of values and a size of groups and then groups the consecutive values
/// stored in a \c Vec.
///
/// For example, if you have an array handle with the six values 0,1,2,3,4,5
/// and give it to a \c ArrayHandleGroupVec with the number of components set
/// to 3, you get an array that looks like it contains two values of \c Vec
/// values of size 3 with the data [0,1,2], [3,4,5].
///
template<typename SourceArrayHandleType, vtkm::IdComponent NUM_COMPONENTS>
class ArrayHandleGroupVec
    : public vtkm::cont::ArrayHandle<
        vtkm::Vec<typename SourceArrayHandleType::ValueType, NUM_COMPONENTS>,
        vtkm::cont::internal::StorageTagGroupVec<
          SourceArrayHandleType, NUM_COMPONENTS> >
{
  VTKM_IS_ARRAY_HANDLE(SourceArrayHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
        ArrayHandleGroupVec,
        (ArrayHandleGroupVec<SourceArrayHandleType, NUM_COMPONENTS>),
        (vtkm::cont::ArrayHandle<
           vtkm::Vec<typename SourceArrayHandleType::ValueType, NUM_COMPONENTS>,
           vtkm::cont::internal::StorageTagGroupVec<
             SourceArrayHandleType, NUM_COMPONENTS> >));

  typedef typename SourceArrayHandleType::ValueType ComponentType;

private:
  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

public:
  VTKM_CONT_EXPORT
  ArrayHandleGroupVec(const SourceArrayHandleType &sourceArray)
    : Superclass(StorageType(sourceArray)) {  }
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleGroupVec_h
