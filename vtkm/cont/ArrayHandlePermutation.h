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
#ifndef vtk_m_ArrayHandlePermutation_h
#define vtk_m_ArrayHandlePermutation_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Assert.h>
#include <vtkm/cont/ErrorControlBadType.h>
#include <vtkm/cont/ErrorControlBadValue.h>

namespace vtkm {
namespace exec {
namespace internal {

template<typename IndexPortalType, typename ValuePortalType>
class ArrayPortalPermutationExec
{
public:
  typedef typename ValuePortalType::ValueType ValueType;

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalPermutationExec( )
    : IndexPortal(), ValuePortal() {  }

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalPermutationExec(
      const IndexPortalType &indexPortal,
      const ValuePortalType &valuePortal)
    : IndexPortal(indexPortal), ValuePortal(valuePortal) {  }

  /// Copy constructor for any other ArrayPortalPermutation with delegate
  /// portal types that can be copied to these portal types. This allows us to
  /// do any type casting that the delegate portals do (like the non-const to
  /// const cast).
  ///
  template<typename OtherIP, typename OtherVP>
  VTKM_EXEC_CONT_EXPORT
  ArrayPortalPermutationExec(
      const ArrayPortalPermutationExec<OtherIP,OtherVP> &src)
    : IndexPortal(src.GetIndexPortal()), ValuePortal(src.GetValuePortal())
  {  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    return this->IndexPortal.GetNumberOfValues();
  }

  VTKM_EXEC_EXPORT
  ValueType Get(vtkm::Id index) const {
    vtkm::Id permutedIndex = this->IndexPortal.Get(index);
    return this->ValuePortal.Get(permutedIndex);
  }

  VTKM_EXEC_EXPORT
  void Set(vtkm::Id index, const ValueType &value) const {
    vtkm::Id permutedIndex = this->IndexPortal.Get(index);
    this->ValuePortal.Set(permutedIndex, value);
  }

  VTKM_EXEC_CONT_EXPORT
  const IndexPortalType &GetIndexPortal() const { return this->IndexPortal; }

  VTKM_EXEC_CONT_EXPORT
  const ValuePortalType &GetValuePortal() const { return this->ValuePortal; }

private:
  IndexPortalType IndexPortal;
  ValuePortalType ValuePortal;
};

}
}
} // namespace vtkm::exec::internal

namespace vtkm {
namespace cont {

namespace internal {

template<typename IndexPortalType, typename ValuePortalType>
class ArrayPortalPermutationCont
{
public:
  typedef typename ValuePortalType::ValueType ValueType;

  VTKM_CONT_EXPORT
  ArrayPortalPermutationCont( )
    : IndexPortal(), ValuePortal() {  }

  VTKM_CONT_EXPORT
  ArrayPortalPermutationCont(
      const IndexPortalType &indexPortal,
      const ValuePortalType &valuePortal)
    : IndexPortal(indexPortal), ValuePortal(valuePortal) {  }

  /// Copy constructor for any other ArrayPortalPermutation with delegate
  /// portal types that can be copied to these portal types. This allows us to
  /// do any type casting that the delegate portals do (like the non-const to
  /// const cast).
  ///
  template<typename OtherIP, typename OtherVP>
  VTKM_CONT_EXPORT
  ArrayPortalPermutationCont(
      const ArrayPortalPermutationCont<OtherIP,OtherVP> &src)
    : IndexPortal(src.GetIndexPortal()), ValuePortal(src.GetValuePortal())
  {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    return this->IndexPortal.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  ValueType Get(vtkm::Id index) const {
    vtkm::Id permutedIndex = this->IndexPortal.Get(index);
    VTKM_ASSERT_CONT(permutedIndex >= 0);
    VTKM_ASSERT_CONT(permutedIndex < this->ValuePortal.GetNumberOfValues());
    return this->ValuePortal.Get(permutedIndex);
  }

  VTKM_CONT_EXPORT
  ValueType Set(vtkm::Id index, const ValueType &value) const {
    vtkm::Id permutedIndex = this->IndexPortal.Get(index);
    VTKM_ASSERT_CONT(permutedIndex >= 0);
    VTKM_ASSERT_CONT(permutedIndex < this->ValuePortal.GetNumberOfValues());
    return this->ValuePortal.Set(permutedIndex, value);
  }

  VTKM_CONT_EXPORT
  const IndexPortalType &GetIndexPortal() const { return this->IndexPortal; }

  VTKM_CONT_EXPORT
  const ValuePortalType &GetValuePortal() const { return this->ValuePortal; }

private:
  IndexPortalType IndexPortal;
  ValuePortalType ValuePortal;
};

template<typename IndexArrayType, typename ValueArrayType>
struct StorageTagPermutation {  };

template<typename IndexArrayType, typename ValueArrayType>
class Storage<
    typename ValueArrayType::ValueType,
    StorageTagPermutation<IndexArrayType, ValueArrayType> >
{
  VTKM_IS_ARRAY_HANDLE(IndexArrayType);
  VTKM_IS_ARRAY_HANDLE(ValueArrayType);

public:
  typedef typename ValueArrayType::ValueType ValueType;

  typedef ArrayPortalPermutationCont<
      typename IndexArrayType::PortalConstControl,
      typename ValueArrayType::PortalControl> PortalType;
  typedef ArrayPortalPermutationCont<
      typename IndexArrayType::PortalConstControl,
      typename ValueArrayType::PortalConstControl> PortalConstType;

  VTKM_CONT_EXPORT
  Storage() : Valid(false) {  }

  VTKM_CONT_EXPORT
  Storage(const IndexArrayType &indexArray, const ValueArrayType &valueArray)
    : IndexArray(indexArray), ValueArray(valueArray), Valid(true) {  }

  VTKM_CONT_EXPORT
  PortalType GetPortal() {
    VTKM_ASSERT_CONT(this->Valid);
    return PortalType(this->IndexArray.GetPortalConstControl(),
                      this->ValueArray.GetPortalControl());
  }

  VTKM_CONT_EXPORT
  PortalConstType GetPortalConst() const {
    VTKM_ASSERT_CONT(this->Valid);
    return PortalConstType(this->IndexArray.GetPortalConstControl(),
                           this->ValueArray.GetPortalConstControl());
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    VTKM_ASSERT_CONT(this->Valid);
    return this->IndexArray.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandlePermutation cannot be allocated.");
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandlePermutation cannot shrink.");
  }

  VTKM_CONT_EXPORT
  void ReleaseResources() {
    // This request is ignored since it is asking to release the resources
    // of the delegate array, which may be used elsewhere. Should the behavior
    // be different?
  }

  VTKM_CONT_EXPORT
  const IndexArrayType &GetIndexArray() const { return this->IndexArray; }

  VTKM_CONT_EXPORT
  const ValueArrayType &GetValueArray() const { return this->ValueArray; }

private:
  IndexArrayType IndexArray;
  ValueArrayType ValueArray;
  bool Valid;
};

template<typename IndexArrayType, typename ValueArrayType, typename Device>
class ArrayTransfer<
    typename ValueArrayType::ValueType,
    StorageTagPermutation<IndexArrayType, ValueArrayType>,
    Device>
{
public:
  typedef typename ValueArrayType::ValueType ValueType;

private:
  typedef StorageTagPermutation<IndexArrayType, ValueArrayType> StorageTag;
  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

public:
  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;

  typedef vtkm::exec::internal::ArrayPortalPermutationExec<
      typename IndexArrayType::template ExecutionTypes<Device>::PortalConst,
      typename ValueArrayType::template ExecutionTypes<Device>::Portal>
    PortalExecution;
  typedef vtkm::exec::internal::ArrayPortalPermutationExec<
      typename IndexArrayType::template ExecutionTypes<Device>::PortalConst,
      typename ValueArrayType::template ExecutionTypes<Device>::PortalConst>
    PortalConstExecution;

  VTKM_CONT_EXPORT
  ArrayTransfer(StorageType *storage)
    : IndexArray(storage->GetIndexArray()),
      ValueArray(storage->GetValueArray()) {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    return this->IndexArray.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData)) {
    return PortalConstExecution(this->IndexArray.PrepareForInput(Device()),
                                this->ValueArray.PrepareForInput(Device()));
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData)) {
    return PortalExecution(this->IndexArray.PrepareForInput(Device()),
                           this->ValueArray.PrepareForInPlace(Device()));
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    if (numberOfValues != this->GetNumberOfValues()) {
      throw vtkm::cont::ErrorControlBadValue(
            "An ArrayHandlePermutation can be used as an output array, "
            "but it cannot be resized. Make sure the index array is sized "
            "to the appropriate length before trying to prepare for output.");
    }

    // We cannot practically allocate ValueArray because we do not know the
    // range of indices. We try to check by seeing if ValueArray has no
    // entries, which clearly indicates that it is not allocated. Otherwise,
    // we have to assume the allocation is correct.
    if ((numberOfValues > 0) && (this->ValueArray.GetNumberOfValues() < 1))
    {
      throw vtkm::cont::ErrorControlBadValue(
            "The value array must be pre-allocated before it is used for the "
            "output of ArrayHandlePermutation.");
    }

    return PortalExecution(this->IndexArray.PrepareForInput(Device()),
                           this->ValueArray.PrepareForOutput(
                             this->ValueArray.GetNumberOfValues(), Device()));
  }

  VTKM_CONT_EXPORT
  void RetrieveOutputData(StorageType *vtkmNotUsed(storage)) const {
    // Implementation of this method should be unnecessary. The internal
    // array handles should automatically retrieve the output data as
    // necessary.
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlBadType(
          "ArrayHandlePermutation cannot shrink.");
  }

  VTKM_CONT_EXPORT
  void ReleaseResources() {
    this->IndexArray.ReleaseResourcesExecution();
    this->ValueArray.ReleaseResourcesExecution();
  }

private:
  IndexArrayType IndexArray;
  ValueArrayType ValueArray;
};

} // namespace internal

/// \brief Implicitly permutes the values in an array.
///
/// ArrayHandlePermutation is a specialization of ArrayHandle. It takes two
/// delegate array handles: an array of indices and an array of values. The
/// array handle created contains the values given permuted by the indices
/// given. So for a given index i, ArrayHandlePermutation looks up the i-th
/// value in the index array to get permuted index j and then gets the j-th
/// value in the value array. This index permutation is done on the fly rather
/// than creating a copy of the array.
///
/// An ArrayHandlePermutation can be used for either input or output. However,
/// if used for output the array must be pre-allocated. That is, the indices
/// must already be established and the values must have an allocation large
/// enough to accommodate the indices. An output ArrayHandlePermutation will
/// only have values changed. The indices are never changed.
///
/// When using ArrayHandlePermutation great care should be taken to make sure
/// that every index in the index array points to a valid position in the value
/// array. Otherwise, access validations will occur. Also, be wary of duplicate
/// indices that point to the same location in the value array. For input
/// arrays, this is fine. However, this could result in unexpected results for
/// using as output and is almost certainly wrong for using as in-place.
///
template <typename IndexArrayHandleType, typename ValueArrayHandleType>
class ArrayHandlePermutation
    : public vtkm::cont::ArrayHandle<
        typename ValueArrayHandleType::ValueType,
        internal::StorageTagPermutation<IndexArrayHandleType, ValueArrayHandleType> >
{
  // If the following line gives a compile error, then the ArrayHandleType
  // template argument is not a valid ArrayHandle type.
  VTKM_IS_ARRAY_HANDLE(IndexArrayHandleType);
  VTKM_IS_ARRAY_HANDLE(ValueArrayHandleType);

public:
  typedef typename ValueArrayHandleType::ValueType ValueType;

  typedef internal::StorageTagPermutation<IndexArrayHandleType, ValueArrayHandleType>
      StorageTag;
  typedef vtkm::cont::internal::Storage<ValueType, StorageTag>
      StorageType;

 public:
  typedef vtkm::cont::ArrayHandle<ValueType, StorageTag> Superclass;

  ArrayHandlePermutation() : Superclass( ) {  }

  ArrayHandlePermutation(const IndexArrayHandleType &indexArray,
                         const ValueArrayHandleType &valueArray)
    : Superclass(StorageType(indexArray, valueArray)) {  }
};

/// make_ArrayHandleTransform is convenience function to generate an
/// ArrayHandleTransform.  It takes in an ArrayHandle and a functor
/// to apply to each element of the Handle.

template <typename IndexArrayHandleType, typename ValueArrayHandleType>
VTKM_CONT_EXPORT
vtkm::cont::ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>
make_ArrayHandlePermutation(IndexArrayHandleType indexArray,
                            ValueArrayHandleType valueArray)
{
  return ArrayHandlePermutation<IndexArrayHandleType,ValueArrayHandleType>(
        indexArray, valueArray);
}

}
} // namespace vtkm::cont

#endif //vtk_m_ArrayHandlePermutation_h
