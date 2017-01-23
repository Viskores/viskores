//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
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
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_cont_ArrayHandleTransform_h
#define vtk_m_cont_ArrayHandleTransform_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorControlBadType.h>
#include <vtkm/cont/ErrorControlInternal.h>

namespace vtkm {
namespace cont {
namespace internal {

/// Tag used in place of an inverse functor.
struct NullFunctorType {};

}
}
} // namespace vtkm::cont::internal

namespace vtkm {
namespace exec {
namespace internal {

typedef vtkm::cont::internal::NullFunctorType NullFunctorType;

/// \brief An array portal that transforms a value from another portal.
///
template<typename ValueType_, typename PortalType_, typename FunctorType_,
  typename InverseFunctorType_=NullFunctorType>
class VTKM_ALWAYS_EXPORT ArrayPortalTransform;

template<typename ValueType_, typename PortalType_, typename FunctorType_>
class VTKM_ALWAYS_EXPORT ArrayPortalTransform<ValueType_,PortalType_,FunctorType_,NullFunctorType>
{
public:
  typedef PortalType_ PortalType;
  typedef ValueType_ ValueType;
  typedef FunctorType_ FunctorType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalTransform(const PortalType &portal = PortalType(),
                       const FunctorType &functor = FunctorType())
    : Portal(portal), Functor(functor)
  {  }

  /// Copy constructor for any other ArrayPortalTransform with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<class OtherV, class OtherP, class OtherF>
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalTransform(const ArrayPortalTransform<OtherV,OtherP,OtherF> &src)
    : Portal(src.GetPortal()),
      Functor(src.GetFunctor())
  {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const {
    return this->Portal.GetNumberOfValues();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const {
    return this->Functor(this->Portal.Get(index));
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const PortalType &GetPortal() const { return this->Portal; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const FunctorType &GetFunctor() const { return this->Functor; }

protected:
  PortalType Portal;
  FunctorType Functor;
};

template<typename ValueType_, typename PortalType_,
  typename FunctorType_, typename InverseFunctorType_>
class VTKM_ALWAYS_EXPORT ArrayPortalTransform : public ArrayPortalTransform<ValueType_,PortalType_,FunctorType_,NullFunctorType>
{
public:
  typedef ArrayPortalTransform<ValueType_,PortalType_,FunctorType_,NullFunctorType> Superclass;
  typedef PortalType_ PortalType;
  typedef ValueType_ ValueType;
  typedef FunctorType_ FunctorType;
  typedef InverseFunctorType_ InverseFunctorType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalTransform(const PortalType &portal = PortalType(),
                       const FunctorType &functor = FunctorType(),
                const InverseFunctorType& inverseFunctor = InverseFunctorType())
    : Superclass(portal,functor), InverseFunctor(inverseFunctor)
  {  }

  template<class OtherV, class OtherP, class OtherF, class OtherInvF>
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalTransform(const ArrayPortalTransform<OtherV,OtherP,OtherF,OtherInvF> &src)
    : Superclass(src), InverseFunctor(src.GetInverseFunctor())
  {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const {
    return this->Portal.Set(index,this->InverseFunctor(value));
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const InverseFunctorType &GetInverseFunctor() const {
    return this->InverseFunctor; }

private:
  InverseFunctorType InverseFunctor;

};

}
}
} // namespace vtkm::exec::internal


namespace vtkm {
namespace cont {

namespace internal {

template<typename ValueType, typename ArrayHandleType, typename FunctorType,
  typename InverseFunctorType=NullFunctorType>
struct VTKM_ALWAYS_EXPORT StorageTagTransform {};

template<typename T, typename ArrayHandleType, typename FunctorType>
class Storage<T, StorageTagTransform<T, ArrayHandleType, FunctorType, NullFunctorType > >
{
public:
  typedef T ValueType;

  // This is meant to be invalid. Because Transform arrays are read only, you
  // should only be able to use the const version.
  struct PortalType
  {
    typedef void *ValueType;
    typedef void *IteratorType;
  };

  typedef vtkm::exec::internal::ArrayPortalTransform<
      ValueType, typename ArrayHandleType::PortalConstControl, FunctorType>
    PortalConstType;

  VTKM_CONT
  Storage() : Valid(false) {  }

  VTKM_CONT
  Storage(const ArrayHandleType &array,
          const FunctorType &functor = FunctorType())
    : Array(array), Functor(functor), Valid(true) {  }

  VTKM_CONT
  PortalType GetPortal() {
    VTKM_ASSERT(this->Valid);
    return PortalType(this->Array.GetPortalControl(),
                      this->Functor);
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const {
    VTKM_ASSERT(this->Valid);
    return PortalConstType(this->Array.GetPortalConstControl(),
                           this->Functor);
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const {
    VTKM_ASSERT(this->Valid);
    return this->Array.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandleTransform is read only. It cannot be allocated.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandleTransform is read only. It cannot shrink.");
  }

  VTKM_CONT
  void ReleaseResources() {
    // This request is ignored since it is asking to release the resources
    // of the delegate array, which may be used elsewhere. Should the behavior
    // be different?
  }

  VTKM_CONT
  const ArrayHandleType &GetArray() const {
    VTKM_ASSERT(this->Valid);
    return this->Array;
  }

  VTKM_CONT
  const FunctorType &GetFunctor() const {
    return this->Functor;
  }

private:
  ArrayHandleType Array;
  FunctorType Functor;
  bool Valid;
};

template<typename T, typename ArrayHandleType, typename FunctorType,
  typename InverseFunctorType>
class Storage<T,
  StorageTagTransform<T, ArrayHandleType, FunctorType, InverseFunctorType> >
{
public:
  typedef T ValueType;

  typedef vtkm::exec::internal::ArrayPortalTransform<ValueType,
    typename ArrayHandleType::PortalControl, FunctorType, InverseFunctorType>
    PortalType;
  typedef vtkm::exec::internal::ArrayPortalTransform<ValueType,
    typename ArrayHandleType::PortalConstControl,FunctorType,InverseFunctorType>
    PortalConstType;

  VTKM_CONT
  Storage() : Valid(false) {  }

  VTKM_CONT
  Storage(const ArrayHandleType &array,
          const FunctorType &functor,
          const InverseFunctorType &inverseFunctor)
    : Array(array), Functor(functor), InverseFunctor(inverseFunctor), Valid(true) {  }

  VTKM_CONT
  PortalType GetPortal() {
    VTKM_ASSERT(this->Valid);
    return PortalType(this->Array.GetPortalControl(),
                      this->Functor,
                      this->InverseFunctor);
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const {
    VTKM_ASSERT(this->Valid);
    return PortalConstType(this->Array.GetPortalConstControl(),
                           this->Functor,
                           this->InverseFunctor);
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const {
    VTKM_ASSERT(this->Valid);
    return this->Array.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues) {
    this->Array.Allocate(numberOfValues);
    this->Valid = true;
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) {
    this->Array.Shrink(numberOfValues);
  }

  VTKM_CONT
  void ReleaseResources() {
    this->Array.ReleaseResources();
    this->Valid = false;
  }

  VTKM_CONT
  const ArrayHandleType &GetArray() const {
    VTKM_ASSERT(this->Valid);
    return this->Array;
  }

  VTKM_CONT
  const FunctorType &GetFunctor() const {
    return this->Functor;
  }

  VTKM_CONT
  const InverseFunctorType &GetInverseFunctor() const {
    return this->InverseFunctor;
  }

private:
  ArrayHandleType Array;
  FunctorType Functor;
  InverseFunctorType InverseFunctor;
  bool Valid;
};

template<typename T,
         typename ArrayHandleType,
         typename FunctorType,
         typename Device>
class ArrayTransfer<
    T, StorageTagTransform<T,ArrayHandleType,FunctorType>, Device>
{
  typedef StorageTagTransform<T,ArrayHandleType,FunctorType> StorageTag;
  typedef vtkm::cont::internal::Storage<T, StorageTag> StorageType;

public:
  typedef T ValueType;

  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;

  //meant to be an invalid writeable execution portal
  typedef typename StorageType::PortalType PortalExecution;
  typedef vtkm::exec::internal::ArrayPortalTransform<
      ValueType,
      typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst,
      FunctorType> PortalConstExecution;

  VTKM_CONT
  ArrayTransfer(StorageType *storage)
    : Array(storage->GetArray()), Functor(storage->GetFunctor()) {  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const {
    return this->Array.GetNumberOfValues();
  }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData)) {
    return PortalConstExecution(this->Array.PrepareForInput(Device()), this->Functor);
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool &vtkmNotUsed(updateData)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandleTransform read only. "
          "Cannot be used for in-place operations.");
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandleTransform read only. Cannot be used as output.");
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType *vtkmNotUsed(storage)) const {
    throw vtkm::cont::ErrorControlInternal(
          "ArrayHandleTransform read only. "
          "There should be no occurance of the ArrayHandle trying to pull "
          "data from the execution environment.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandleTransform read only. Cannot shrink.");
  }

  VTKM_CONT
  void ReleaseResources() {
    this->Array.ReleaseResourcesExecution();
  }

private:
  ArrayHandleType Array;
  FunctorType Functor;
};

template<typename T,
         typename ArrayHandleType,
         typename FunctorType,
         typename InverseFunctorType,
         typename Device>
class ArrayTransfer<
  T, StorageTagTransform<T,ArrayHandleType,FunctorType,InverseFunctorType>,
  Device>
{
  typedef StorageTagTransform<T,ArrayHandleType,
                              FunctorType,InverseFunctorType> StorageTag;
  typedef vtkm::cont::internal::Storage<T, StorageTag> StorageType;

public:
  typedef T ValueType;

  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;

  typedef vtkm::exec::internal::ArrayPortalTransform<
      ValueType,
      typename ArrayHandleType::template ExecutionTypes<Device>::Portal,
      FunctorType, InverseFunctorType> PortalExecution;
  typedef vtkm::exec::internal::ArrayPortalTransform<
      ValueType,
      typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst,
      FunctorType, InverseFunctorType> PortalConstExecution;

  VTKM_CONT
  ArrayTransfer(StorageType *storage)
    : Array(storage->GetArray()),
      Functor(storage->GetFunctor()),
      InverseFunctor(storage->GetInverseFunctor()) {  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const {
    return this->Array.GetNumberOfValues();
  }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData)) {
    return PortalConstExecution(this->Array.PrepareForInput(Device()),this->Functor,this->InverseFunctor);
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool &vtkmNotUsed(updateData)) {
    return PortalExecution(this->Array.PrepareForInPlace(Device()),this->Functor,this->InverseFunctor);
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues) {
    return PortalExecution(this->Array.PrepareForOutput(numberOfValues,
                                                        Device()),this->Functor,this->InverseFunctor);
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType *vtkmNotUsed(storage)) const {
    // Implementation of this method should be unnecessary. The internal
    // array handle should automatically retrieve the output data as necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) {
    this->Array.Shrink(numberOfValues);
  }

  VTKM_CONT
  void ReleaseResources() {
    this->Array.ReleaseResourcesExecution();
  }

private:
  ArrayHandleType Array;
  FunctorType Functor;
  InverseFunctorType InverseFunctor;
};

} // namespace internal

/// \brief Implicitly transform values of one array to another with a functor.
///
/// ArrayHandleTransforms is a specialization of ArrayHandle. It takes a
/// delegate array handle and makes a new handle that calls a given unary
/// functor with the element at a given index and returns the result of the
/// functor as the value of this array at that position. This transformation is
/// done on demand. That is, rather than make a new copy of the array with new
/// values, the transformation is done as values are read from the array. Thus,
/// the functor operator should work in both the control and execution
/// environments.
///
template <typename T,
          typename ArrayHandleType,
          typename FunctorType,
          typename InverseFunctorType=internal::NullFunctorType>
class ArrayHandleTransform;

template <typename T,
          typename ArrayHandleType,
          typename FunctorType>
class ArrayHandleTransform<T,ArrayHandleType,FunctorType,internal::NullFunctorType>
    : public vtkm::cont::ArrayHandle<
        T, internal::StorageTagTransform<T, ArrayHandleType, FunctorType> >
{
  // If the following line gives a compile error, then the ArrayHandleType
  // template argument is not a valid ArrayHandle type.
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
      ArrayHandleTransform,
      (ArrayHandleTransform<T,ArrayHandleType,FunctorType>),
      (vtkm::cont::ArrayHandle<
         T, internal::StorageTagTransform<T, ArrayHandleType, FunctorType> >));

private:
  typedef vtkm::cont::internal::Storage<T, StorageTag> StorageType;

public:
  VTKM_CONT
  ArrayHandleTransform(const ArrayHandleType &handle,
                       const FunctorType &functor = FunctorType())
    : Superclass(StorageType(handle, functor)) {  }
};

/// make_ArrayHandleTransform is convenience function to generate an
/// ArrayHandleTransform.  It takes in an ArrayHandle and a functor
/// to apply to each element of the Handle.

template <typename T, typename HandleType, typename FunctorType>
VTKM_CONT
vtkm::cont::ArrayHandleTransform<T, HandleType, FunctorType>
make_ArrayHandleTransform(HandleType handle, FunctorType functor)
{
  return ArrayHandleTransform<T,HandleType,FunctorType>(handle,functor);
}

// ArrayHandleTransform with inverse functors enabled (no need to subclass from
// ArrayHandleTransform without inverse functors: nothing to inherit).
template <typename T,
          typename ArrayHandleType,
          typename FunctorType,
          typename InverseFunctorType>
class ArrayHandleTransform
    : public vtkm::cont::ArrayHandle<
        T,
        internal::StorageTagTransform<T, ArrayHandleType, FunctorType,
          InverseFunctorType> >
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
      ArrayHandleTransform,
      (ArrayHandleTransform<T,ArrayHandleType,FunctorType,InverseFunctorType>),
      (vtkm::cont::ArrayHandle<
       T, internal::StorageTagTransform<T, ArrayHandleType, FunctorType,
       InverseFunctorType> >));

private:
  typedef vtkm::cont::internal::Storage<T, StorageTag> StorageType;

 public:
  ArrayHandleTransform(const ArrayHandleType &handle,
                       const FunctorType &functor = FunctorType(),
                const InverseFunctorType &inverseFunctor = InverseFunctorType())
    : Superclass(StorageType(handle, functor, inverseFunctor)) {  }
};

template <typename T, typename HandleType, typename FunctorType, typename InverseFunctorType>
VTKM_CONT
vtkm::cont::ArrayHandleTransform<T, HandleType, FunctorType, InverseFunctorType>
make_ArrayHandleTransform(HandleType handle, FunctorType functor, InverseFunctorType inverseFunctor)
{
  return ArrayHandleTransform<T,HandleType,FunctorType,InverseFunctorType>(handle,functor,inverseFunctor);
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleTransform_h
