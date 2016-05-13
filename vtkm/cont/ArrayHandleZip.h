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
#ifndef vtk_m_cont_ArrayHandleZip_h
#define vtk_m_cont_ArrayHandleZip_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/Pair.h>

namespace vtkm {
namespace exec {
namespace internal {

/// \brief An array portal that zips two portals together into a single value
/// for the execution environment
template<typename ValueType_,
         typename PortalTypeFirst_,
         typename PortalTypeSecond_>
class ArrayPortalExecZip
{
public:
  typedef ValueType_ ValueType;
  typedef ValueType_ IteratorType;
  typedef PortalTypeFirst_ PortalTypeFirst;
  typedef PortalTypeSecond_ PortalTypeSecond;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  ArrayPortalExecZip()
    : PortalFirst(), PortalSecond()
  {  } //needs to be host and device so that cuda can create lvalue of these

  VTKM_CONT_EXPORT
  ArrayPortalExecZip(const PortalTypeFirst  &portalfirst,
                     const PortalTypeSecond &portalsecond)
    : PortalFirst(portalfirst), PortalSecond(portalsecond)
  {  }

  /// Copy constructor for any other ArrayPortalExecZip with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<class OtherV, class OtherF, class OtherS>
  VTKM_CONT_EXPORT
  ArrayPortalExecZip(const ArrayPortalExecZip<OtherV,OtherF,OtherS> &src)
    : PortalFirst(src.GetFirstPortal()),
      PortalSecond(src.GetSecondPortal())
  {  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const { return this->PortalFirst.GetNumberOfValues(); }

  VTKM_EXEC_EXPORT
  ValueType Get(vtkm::Id index) const {
    return vtkm::make_Pair(this->PortalFirst.Get(index),
                          this->PortalSecond.Get(index));
  }

  VTKM_EXEC_EXPORT
  void Set(vtkm::Id index, const ValueType &value) const {
    this->PortalFirst.Set(index, value.first);
    this->PortalSecond.Set(index, value.second);
  }

  VTKM_EXEC_CONT_EXPORT
  const PortalTypeFirst &GetFirstPortal() const { return this->PortalFirst; }

  VTKM_EXEC_CONT_EXPORT
  const PortalTypeSecond &GetSecondPortal() const { return this->PortalSecond; }


private:
  PortalTypeFirst PortalFirst;
  PortalTypeSecond PortalSecond;
};

}
}
} // namespace vtkm::exec::internal


namespace vtkm {
namespace cont {

namespace internal {

/// \brief An array portal that zips two portals together into a single value
/// for the control environment
template<typename ValueType_,
         typename PortalTypeFirst,
         typename PortalTypeSecond>
class ArrayPortalContZip
{
public:
  typedef ValueType_ ValueType;
  typedef ValueType_ IteratorType;

  VTKM_CONT_EXPORT
  ArrayPortalContZip(const PortalTypeFirst  &portalfirst = PortalTypeFirst(),
                 const PortalTypeSecond &portalsecond = PortalTypeSecond())
    : PortalFirst(portalfirst), PortalSecond(portalsecond)
  {  }

  /// Copy constructor for any other ArrayPortalContZip with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<class OtherV, class OtherF, class OtherS>
  VTKM_CONT_EXPORT
  ArrayPortalContZip(const ArrayPortalContZip<OtherV,OtherF,OtherS> &src)
    : PortalFirst(src.GetFirstPortal()),
      PortalSecond(src.GetSecondPortal())
  {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const { return this->PortalFirst.GetNumberOfValues(); }

  VTKM_CONT_EXPORT
  ValueType Get(vtkm::Id index) const {
    return vtkm::make_Pair(this->PortalFirst.Get(index),
                          this->PortalSecond.Get(index));
  }

  VTKM_CONT_EXPORT
  void Set(vtkm::Id index, const ValueType &value) const {
    this->PortalFirst.Set(index, value.first);
    this->PortalSecond.Set(index, value.second);
  }

  VTKM_CONT_EXPORT
  const PortalTypeFirst &GetFirstPortal() const { return this->PortalFirst; }

  VTKM_CONT_EXPORT
  const PortalTypeSecond &GetSecondPortal() const { return this->PortalSecond; }


private:
  PortalTypeFirst PortalFirst;
  PortalTypeSecond PortalSecond;
};

template<typename FirstHandleType, typename SecondHandleType>
struct StorageTagZip {  };

/// This helper struct defines the value type for a zip container containing
/// the given two array handles.
///
template<typename FirstHandleType, typename SecondHandleType>
struct ArrayHandleZipTraits {
  /// The ValueType (a pair containing the value types of the two arrays).
  ///
  typedef vtkm::Pair<typename FirstHandleType::ValueType,
                    typename SecondHandleType::ValueType> ValueType;

  /// The appropriately templated tag.
  ///
  typedef StorageTagZip<FirstHandleType,SecondHandleType> Tag;

  /// The superclass for ArrayHandleZip.
  ///
  typedef vtkm::cont::ArrayHandle<ValueType,Tag> Superclass;
};


template<typename T, typename FirstHandleType, typename SecondHandleType>
class Storage<T, StorageTagZip<FirstHandleType, SecondHandleType > >
{
  VTKM_IS_ARRAY_HANDLE(FirstHandleType);
  VTKM_IS_ARRAY_HANDLE(SecondHandleType);

public:
  typedef T ValueType;

  typedef ArrayPortalContZip< ValueType,
                          typename FirstHandleType::PortalControl,
                          typename SecondHandleType::PortalControl> PortalType;
  typedef ArrayPortalContZip< ValueType,
                          typename FirstHandleType::PortalConstControl,
                          typename SecondHandleType::PortalConstControl>
                                                               PortalConstType;

  VTKM_CONT_EXPORT
  Storage() : FirstArray(), SecondArray() {  }

  VTKM_CONT_EXPORT
  Storage(const FirstHandleType &farray, const SecondHandleType &sarray)
    : FirstArray(farray), SecondArray(sarray)
  {

  }

  VTKM_CONT_EXPORT
  PortalType GetPortal() {
    return PortalType(this->FirstArray.GetPortalControl(),
                      this->SecondArray.GetPortalControl());
  }

  VTKM_CONT_EXPORT
  PortalConstType GetPortalConst() const {
    return PortalConstType(this->FirstArray.GetPortalConstControl(),
                           this->SecondArray.GetPortalConstControl());
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    VTKM_ASSERT(this->FirstArray.GetNumberOfValues()
                     == this->SecondArray.GetNumberOfValues());
    return this->FirstArray.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  void Allocate(vtkm::Id numberOfValues) {
    this->FirstArray.Allocate(numberOfValues);
    this->SecondArray.Allocate(numberOfValues);
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id numberOfValues) {
    this->FirstArray.Shrink(numberOfValues);
    this->SecondArray.Shrink(numberOfValues);
  }

  VTKM_CONT_EXPORT
  void ReleaseResources() {
    // This request is ignored since it is asking to release the resources
    // of the two zipped array, which may be used elsewhere.
  }

  VTKM_CONT_EXPORT
  const FirstHandleType &GetFirstArray() const {
    return this->FirstArray;
  }

  VTKM_CONT_EXPORT
  const SecondHandleType &GetSecondArray() const {
    return this->SecondArray;
  }

private:
  FirstHandleType FirstArray;
  SecondHandleType SecondArray;
};

template<typename T,
         typename FirstHandleType,
         typename SecondHandleType,
         typename Device>
class ArrayTransfer<
    T, StorageTagZip<FirstHandleType,SecondHandleType>, Device>
{
  typedef StorageTagZip<FirstHandleType,SecondHandleType> StorageTag;
  typedef vtkm::cont::internal::Storage<T, StorageTag> StorageType;

public:
  typedef T ValueType;

  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;

  typedef vtkm::exec::internal::ArrayPortalExecZip<
      ValueType,
      typename FirstHandleType::template ExecutionTypes<Device>::Portal,
      typename SecondHandleType::template ExecutionTypes<Device>::Portal
      > PortalExecution;

  typedef vtkm::exec::internal::ArrayPortalExecZip<
      ValueType,
      typename FirstHandleType::template ExecutionTypes<Device>::PortalConst,
      typename SecondHandleType::template ExecutionTypes<Device>::PortalConst
      > PortalConstExecution;

  VTKM_CONT_EXPORT
  ArrayTransfer(StorageType *storage)
    :  FirstArray(storage->GetFirstArray()),
       SecondArray(storage->GetSecondArray())
    {  }


  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT( this->FirstArray.GetNumberOfValues()
                      == this->SecondArray.GetNumberOfValues() );
    return this->FirstArray.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData)) {
    return PortalConstExecution(this->FirstArray.PrepareForInput(Device()),
                                this->SecondArray.PrepareForInput(Device()));
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData)) {
    const vtkm::Id numberOfValues = this->GetNumberOfValues();
    return PortalExecution(this->FirstArray.PrepareForOutput(numberOfValues, Device()),
                           this->SecondArray.PrepareForOutput(numberOfValues, Device()));
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return PortalExecution( this->FirstArray.PrepareForOutput(numberOfValues, Device()),
                            this->SecondArray.PrepareForOutput(numberOfValues, Device()) );
  }

  VTKM_CONT_EXPORT
  void RetrieveOutputData(StorageType *vtkmNotUsed(storage)) const {
    // Implementation of this method should be unnecessary. The internal
    // first and second array handles should automatically retrieve the
    // output data as necessary.
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id numberOfValues) {
    this->FirstArray.Shrink(numberOfValues);
    this->SecondArray.Shrink(numberOfValues);
  }

  VTKM_CONT_EXPORT
  void ReleaseResources() {
    this->FirstArray.ReleaseResourcesExecution();
    this->SecondArray.ReleaseResourcesExecution();
  }


private:
  FirstHandleType FirstArray;
  SecondHandleType SecondArray;
};
} // namespace internal

/// ArrayHandleZip is a specialization of ArrayHandle. It takes two delegate
/// array handle and makes a new handle that access the corresponding entries
/// in these arrays as a pair.
///
template<typename FirstHandleType,
         typename SecondHandleType>
class ArrayHandleZip
    : public internal::ArrayHandleZipTraits<FirstHandleType,SecondHandleType>::Superclass
{
  // If the following line gives a compile error, then the FirstHandleType
  // template argument is not a valid ArrayHandle type.
  VTKM_IS_ARRAY_HANDLE(FirstHandleType);

  // If the following line gives a compile error, then the SecondHandleType
  // template argument is not a valid ArrayHandle type.
  VTKM_IS_ARRAY_HANDLE(SecondHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
      ArrayHandleZip,
      (ArrayHandleZip<FirstHandleType,SecondHandleType>),
      (typename internal::ArrayHandleZipTraits<
         FirstHandleType,SecondHandleType>::Superclass));

private:
  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

public:
  VTKM_CONT_EXPORT
  ArrayHandleZip(const FirstHandleType &firstArray,
                 const SecondHandleType &secondArray)
    : Superclass( StorageType( firstArray, secondArray ) ) { }
};

/// A convenience function for creating an ArrayHandleZip. It takes the two
/// arrays to be zipped together.
///
template<typename FirstHandleType, typename SecondHandleType>
VTKM_CONT_EXPORT
vtkm::cont::ArrayHandleZip<FirstHandleType,SecondHandleType>
make_ArrayHandleZip(const FirstHandleType &first,
                    const SecondHandleType &second)
{
  return ArrayHandleZip<FirstHandleType,SecondHandleType>(first, second);
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleZip_h
