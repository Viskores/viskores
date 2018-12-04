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
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_cont_ArrayHandlePermutation_h
#define vtk_m_cont_ArrayHandlePermutation_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorBadValue.h>

namespace vtkm
{
namespace exec
{
namespace internal
{

template <typename IndexPortalType, typename ValuePortalType>
class VTKM_ALWAYS_EXPORT ArrayPortalPermutation
{
public:
  using ValueType = typename ValuePortalType::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalPermutation()
    : IndexPortal()
    , ValuePortal()
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalPermutation(const IndexPortalType& indexPortal, const ValuePortalType& valuePortal)
    : IndexPortal(indexPortal)
    , ValuePortal(valuePortal)
  {
  }

  /// Copy constructor for any other ArrayPortalPermutation with delegate
  /// portal types that can be copied to these portal types. This allows us to
  /// do any type casting that the delegate portals do (like the non-const to
  /// const cast).
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OtherIP, typename OtherVP>
  VTKM_EXEC_CONT ArrayPortalPermutation(const ArrayPortalPermutation<OtherIP, OtherVP>& src)
    : IndexPortal(src.GetIndexPortal())
    , ValuePortal(src.GetValuePortal())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->IndexPortal.GetNumberOfValues(); }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Get(vtkm::Id index) const
  {
    vtkm::Id permutedIndex = this->IndexPortal.Get(index);
    return this->ValuePortal.Get(permutedIndex);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void Set(vtkm::Id index, const ValueType& value) const
  {
    vtkm::Id permutedIndex = this->IndexPortal.Get(index);
    this->ValuePortal.Set(permutedIndex, value);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const IndexPortalType& GetIndexPortal() const { return this->IndexPortal; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const ValuePortalType& GetValuePortal() const { return this->ValuePortal; }

private:
  IndexPortalType IndexPortal;
  ValuePortalType ValuePortal;
};
}
}
} // namespace vtkm::exec::internal

namespace vtkm
{
namespace cont
{

namespace internal
{

template <typename IndexArrayType, typename ValueArrayType>
struct VTKM_ALWAYS_EXPORT StorageTagPermutation
{
};

template <typename IndexArrayType, typename ValueArrayType>
class Storage<typename ValueArrayType::ValueType,
              StorageTagPermutation<IndexArrayType, ValueArrayType>>
{
  VTKM_IS_ARRAY_HANDLE(IndexArrayType);
  VTKM_IS_ARRAY_HANDLE(ValueArrayType);

public:
  using ValueType = typename ValueArrayType::ValueType;

  using PortalType =
    vtkm::exec::internal::ArrayPortalPermutation<typename IndexArrayType::PortalConstControl,
                                                 typename ValueArrayType::PortalControl>;
  using PortalConstType =
    vtkm::exec::internal::ArrayPortalPermutation<typename IndexArrayType::PortalConstControl,
                                                 typename ValueArrayType::PortalConstControl>;

  VTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  VTKM_CONT
  Storage(const IndexArrayType& indexArray, const ValueArrayType& valueArray)
    : IndexArray(indexArray)
    , ValueArray(valueArray)
    , Valid(true)
  {
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    VTKM_ASSERT(this->Valid);
    return PortalType(this->IndexArray.GetPortalConstControl(),
                      this->ValueArray.GetPortalControl());
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Valid);
    return PortalConstType(this->IndexArray.GetPortalConstControl(),
                           this->ValueArray.GetPortalConstControl());
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Valid);
    return this->IndexArray.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayHandlePermutation cannot be allocated.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayHandlePermutation cannot shrink.");
  }

  VTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since it is asking to release the resources
    // of the delegate array, which may be used elsewhere. Should the behavior
    // be different?
  }

  VTKM_CONT
  const IndexArrayType& GetIndexArray() const { return this->IndexArray; }

  VTKM_CONT
  const ValueArrayType& GetValueArray() const { return this->ValueArray; }

private:
  IndexArrayType IndexArray;
  ValueArrayType ValueArray;
  bool Valid;
};

template <typename IndexArrayType, typename ValueArrayType, typename Device>
class ArrayTransfer<typename ValueArrayType::ValueType,
                    StorageTagPermutation<IndexArrayType, ValueArrayType>,
                    Device>
{
public:
  using ValueType = typename ValueArrayType::ValueType;

private:
  using StorageTag = StorageTagPermutation<IndexArrayType, ValueArrayType>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = vtkm::exec::internal::ArrayPortalPermutation<
    typename IndexArrayType::template ExecutionTypes<Device>::PortalConst,
    typename ValueArrayType::template ExecutionTypes<Device>::Portal>;
  using PortalConstExecution = vtkm::exec::internal::ArrayPortalPermutation<
    typename IndexArrayType::template ExecutionTypes<Device>::PortalConst,
    typename ValueArrayType::template ExecutionTypes<Device>::PortalConst>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : IndexArray(storage->GetIndexArray())
    , ValueArray(storage->GetValueArray())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->IndexArray.GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->IndexArray.PrepareForInput(Device()),
                                this->ValueArray.PrepareForInput(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return PortalExecution(this->IndexArray.PrepareForInput(Device()),
                           this->ValueArray.PrepareForInPlace(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    if (numberOfValues != this->GetNumberOfValues())
    {
      throw vtkm::cont::ErrorBadValue(
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
      throw vtkm::cont::ErrorBadValue(
        "The value array must be pre-allocated before it is used for the "
        "output of ArrayHandlePermutation.");
    }

    return PortalExecution(
      this->IndexArray.PrepareForInput(Device()),
      this->ValueArray.PrepareForOutput(this->ValueArray.GetNumberOfValues(), Device()));
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handles should automatically retrieve the output data as
    // necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayHandlePermutation cannot shrink.");
  }

  VTKM_CONT
  void ReleaseResources()
  {
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
      internal::StorageTagPermutation<IndexArrayHandleType, ValueArrayHandleType>>
{
  // If the following line gives a compile error, then the ArrayHandleType
  // template argument is not a valid ArrayHandle type.
  VTKM_IS_ARRAY_HANDLE(IndexArrayHandleType);
  VTKM_IS_ARRAY_HANDLE(ValueArrayHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandlePermutation,
    (ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>),
    (vtkm::cont::ArrayHandle<
      typename ValueArrayHandleType::ValueType,
      internal::StorageTagPermutation<IndexArrayHandleType, ValueArrayHandleType>>));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandlePermutation(const IndexArrayHandleType& indexArray,
                         const ValueArrayHandleType& valueArray)
    : Superclass(StorageType(indexArray, valueArray))
  {
  }
};

/// make_ArrayHandleTransform is convenience function to generate an
/// ArrayHandleTransform.  It takes in an ArrayHandle and a functor
/// to apply to each element of the Handle.

template <typename IndexArrayHandleType, typename ValueArrayHandleType>
VTKM_CONT vtkm::cont::ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>
make_ArrayHandlePermutation(IndexArrayHandleType indexArray, ValueArrayHandleType valueArray)
{
  return ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>(indexArray, valueArray);
}
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
namespace vtkm
{
namespace cont
{

template <typename IdxAH, typename ValAH>
struct TypeString<vtkm::cont::ArrayHandlePermutation<IdxAH, ValAH>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name =
      "AH_Permutation<" + TypeString<IdxAH>::Get() + "," + TypeString<ValAH>::Get() + ">";
    return name;
  }
};

template <typename IdxAH, typename ValAH>
struct TypeString<
  vtkm::cont::ArrayHandle<typename ValAH::ValueType,
                          vtkm::cont::internal::StorageTagPermutation<IdxAH, ValAH>>>
  : TypeString<vtkm::cont::ArrayHandlePermutation<IdxAH, ValAH>>
{
};
}
} // vtkm::cont

namespace diy
{

template <typename IdxAH, typename ValAH>
struct Serialization<vtkm::cont::ArrayHandlePermutation<IdxAH, ValAH>>
{
private:
  using Type = vtkm::cont::ArrayHandlePermutation<IdxAH, ValAH>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    diy::save(bb, storage.GetIndexArray());
    diy::save(bb, storage.GetValueArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    IdxAH indices;
    ValAH values;

    diy::load(bb, indices);
    diy::load(bb, values);

    obj = vtkm::cont::make_ArrayHandlePermutation(indices, values);
  }
};

template <typename IdxAH, typename ValAH>
struct Serialization<
  vtkm::cont::ArrayHandle<typename ValAH::ValueType,
                          vtkm::cont::internal::StorageTagPermutation<IdxAH, ValAH>>>
  : Serialization<vtkm::cont::ArrayHandlePermutation<IdxAH, ValAH>>
{
};

} // diy

#endif //vtk_m_cont_ArrayHandlePermutation_h
