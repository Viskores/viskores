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
//  Copyright 2015. Los Alamos National Security
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
#include <vtkm/cont/Assert.h>
#include <vtkm/cont/ErrorControlBadType.h>
#include <vtkm/cont/ErrorControlInternal.h>

namespace vtkm {
namespace exec {
namespace internal {
/// \brief An array portal that transforms a value from another portal.
///
template<typename ValueType_, typename PortalType_, typename FunctorType_>
class ArrayPortalExecTransform
{
public:
  typedef PortalType_ PortalType;
  typedef ValueType_ ValueType;
  typedef FunctorType_ FunctorType;

  VTKM_CONT_EXPORT
  ArrayPortalExecTransform(const PortalType &portal = PortalType(),
                       const FunctorType &functor = FunctorType())
    : Portal(portal), Functor(functor)
  {  }

  /// Copy constructor for any other ArrayPortalExecTransform with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<class OtherV, class OtherP, class OtherF>
  VTKM_CONT_EXPORT
  ArrayPortalExecTransform(const ArrayPortalExecTransform<OtherV,OtherP,OtherF> &src)
    : Portal(src.GetPortal()),
      Functor(src.GetFunctor())
  {  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    return this->Portal.GetNumberOfValues();
  }

  VTKM_EXEC_EXPORT
  ValueType Get(vtkm::Id index) const {
    return this->Functor(this->Portal.Get(index));
  }

  VTKM_EXEC_CONT_EXPORT
  const PortalType &GetPortal() const { return this->Portal; }

  VTKM_EXEC_CONT_EXPORT
  const FunctorType &GetFunctor() const { return this->Functor; }

private:
  PortalType Portal;
  FunctorType Functor;
};

}
}
} // namespace vtkm::exec::internal


namespace vtkm {
namespace cont {

namespace internal {

/// \brief An array portal that transforms a value from another portal.
///
template<typename ValueType_, typename PortalType_, typename FunctorType_>
class ArrayPortalContTransform
{
public:
  typedef PortalType_ PortalType;
  typedef ValueType_ ValueType;
  typedef FunctorType_ FunctorType;

  VTKM_CONT_EXPORT
  ArrayPortalContTransform(const PortalType &portal = PortalType(),
                       const FunctorType &functor = FunctorType())
    : Portal(portal), Functor(functor)
  {  }

  /// Copy constructor for any other ArrayPortalContTransform with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<class OtherV, class OtherP, class OtherF>
  VTKM_CONT_EXPORT
  ArrayPortalContTransform(const ArrayPortalContTransform<OtherV,OtherP,OtherF> &src)
    : Portal(src.GetPortal()),
      Functor(src.GetFunctor())
  {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    return this->Portal.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  ValueType Get(vtkm::Id index) const {
    return this->Functor(this->Portal.Get(index));
  }

  VTKM_CONT_EXPORT
  const PortalType &GetPortal() const { return this->Portal; }

  VTKM_CONT_EXPORT
  const FunctorType &GetFunctor() const { return this->Functor; }

private:
  PortalType Portal;
  FunctorType Functor;
};

template<typename ValueType, typename ArrayHandleType, typename FunctorType>
struct StorageTagTransform;

template<typename T, typename ArrayHandleType, typename FunctorType>
class Storage<T, StorageTagTransform<T, ArrayHandleType, FunctorType > >
{
public:
  typedef T ValueType;

  typedef ArrayPortalContTransform<
      ValueType, typename ArrayHandleType::PortalControl, FunctorType>
    PortalType;
  typedef ArrayPortalContTransform<
      ValueType, typename ArrayHandleType::PortalConstControl, FunctorType>
    PortalConstType;

  VTKM_CONT_EXPORT
  Storage() : Valid(false) {  }

  VTKM_CONT_EXPORT
  Storage(const ArrayHandleType &array,
          const FunctorType &functor = FunctorType())
    : Array(array), Functor(functor), Valid(true) {  }

  VTKM_CONT_EXPORT
  PortalType GetPortal() {
    VTKM_ASSERT_CONT(this->Valid);
    return PortalType(this->Array.GetPortalControl(),
                      this->Functor);
  }

  VTKM_CONT_EXPORT
  PortalConstType GetPortalConst() const {
    VTKM_ASSERT_CONT(this->Valid);
    return PortalConstType(this->Array.GetPortalConstControl(),
                           this->Functor);
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    VTKM_ASSERT_CONT(this->Valid);
    return this->Array.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandleTransform is read only. It cannot be allocated.");
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandleTransform is read only. It cannot shrink.");
  }

  VTKM_CONT_EXPORT
  void ReleaseResources() {
    // This request is ignored since it is asking to release the resources
    // of the delegate array, which may be used elsewhere. Should the behavior
    // be different?
  }

  VTKM_CONT_EXPORT
  const ArrayHandleType &GetArray() const {
    VTKM_ASSERT_CONT(this->Valid);
    return this->Array;
  }

private:
  ArrayHandleType Array;
  FunctorType Functor;
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

  typedef vtkm::exec::internal::ArrayPortalExecTransform<
      ValueType,
      typename ArrayHandleType::template ExecutionTypes<Device>::Portal,
      FunctorType> PortalExecution;
  typedef vtkm::exec::internal::ArrayPortalExecTransform<
      ValueType,
      typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst,
      FunctorType> PortalConstExecution;

  VTKM_CONT_EXPORT
  ArrayTransfer(StorageType *storage) : Array(storage->GetArray()) {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    return this->Array.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData)) {
    return PortalConstExecution(this->Array.PrepareForInput(Device()));
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForInPlace(bool &vtkmNotUsed(updateData)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandleTransform read only. "
          "Cannot be used for in-place operations.");
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandleTransform read only. Cannot be used as output.");
  }

  VTKM_CONT_EXPORT
  void RetrieveOutputData(StorageType *vtkmNotUsed(storage)) const {
    throw vtkm::cont::ErrorControlInternal(
          "ArrayHandleTransform read only. "
          "There should be no occurance of the ArrayHandle trying to pull "
          "data from the execution environment.");
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandleTransform read only. Cannot shrink.");
  }

  VTKM_CONT_EXPORT
  void ReleaseResources() {
    this->Array.ReleaseResourcesExecution();
  }

private:
  ArrayHandleType Array;
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
template <typename ValueType,
          typename ArrayHandleType,
          typename FunctorType>
class ArrayHandleTransform
    : public vtkm::cont::ArrayHandle<
        ValueType,
        internal::StorageTagTransform<ValueType, ArrayHandleType, FunctorType> >
{
  // If the following line gives a compile error, then the ArrayHandleType
  // template argument is not a valid ArrayHandle type.
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  typedef internal::StorageTagTransform<ValueType, ArrayHandleType, FunctorType>
      StorageTag;
  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

 public:
  typedef vtkm::cont::ArrayHandle<ValueType, StorageTag> Superclass;

  ArrayHandleTransform() : Superclass( ) {  }

  ArrayHandleTransform(const ArrayHandleType &handle,
                       const FunctorType &functor = FunctorType())
    : Superclass(StorageType(handle, functor)) {  }
};

/// make_ArrayHandleTransform is convenience function to generate an
/// ArrayHandleTransform.  It takes in an ArrayHandle and a functor
/// to apply to each element of the Handle.

template <typename T, typename HandleType, typename FunctorType>
VTKM_CONT_EXPORT
vtkm::cont::ArrayHandleTransform<T, HandleType, FunctorType>
make_ArrayHandleTransform(HandleType handle, FunctorType functor)
{
  return ArrayHandleTransform<T,HandleType,FunctorType>(handle,functor);
}


}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleTransform_h
