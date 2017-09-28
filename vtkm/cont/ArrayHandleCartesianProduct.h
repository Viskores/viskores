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
#ifndef vtk_m_cont_ArrayHandleCartesianProduct_h
#define vtk_m_cont_ArrayHandleCartesianProduct_h

#include <vtkm/Assert.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadAllocation.h>

namespace vtkm
{
namespace exec
{
namespace internal
{

/// \brief An array portal that acts as a 3D cartesian product of 3 arrays.
///
template <typename ValueType_,
          typename PortalTypeFirst_,
          typename PortalTypeSecond_,
          typename PortalTypeThird_>
class VTKM_ALWAYS_EXPORT ArrayPortalCartesianProduct
{
public:
  using ValueType = ValueType_;
  using IteratorType = ValueType_;
  using PortalTypeFirst = PortalTypeFirst_;
  using PortalTypeSecond = PortalTypeSecond_;
  using PortalTypeThird = PortalTypeThird_;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalCartesianProduct()
    : PortalFirst()
    , PortalSecond()
    , PortalThird()
  {
  } //needs to be host and device so that cuda can create lvalue of these

  VTKM_CONT
  ArrayPortalCartesianProduct(const PortalTypeFirst& portalfirst,
                              const PortalTypeSecond& portalsecond,
                              const PortalTypeThird& portalthird)
    : PortalFirst(portalfirst)
    , PortalSecond(portalsecond)
    , PortalThird(portalthird)
  {
  }

  /// Copy constructor for any other ArrayPortalCartesianProduct with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///

  template <class OtherV, class OtherP1, class OtherP2, class OtherP3>
  VTKM_CONT ArrayPortalCartesianProduct(
    const ArrayPortalCartesianProduct<OtherV, OtherP1, OtherP2, OtherP3>& src)
    : PortalFirst(src.GetPortalFirst())
    , PortalSecond(src.GetPortalSecond())
    , PortalThird(src.GetPortalThird())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->PortalFirst.GetNumberOfValues() * this->PortalSecond.GetNumberOfValues() *
      this->PortalThird.GetNumberOfValues();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->GetNumberOfValues());

    vtkm::Id dim1 = this->PortalFirst.GetNumberOfValues();
    vtkm::Id dim2 = this->PortalSecond.GetNumberOfValues();
    vtkm::Id dim12 = dim1 * dim2;
    vtkm::Id idx12 = index % dim12;
    vtkm::Id i1 = idx12 % dim1;
    vtkm::Id i2 = idx12 / dim1;
    vtkm::Id i3 = index / dim12;

    return vtkm::make_Vec(
      this->PortalFirst.Get(i1), this->PortalSecond.Get(i2), this->PortalThird.Get(i3));
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->GetNumberOfValues());

    vtkm::Id dim1 = this->PortalFirst.GetNumberOfValues();
    vtkm::Id dim2 = this->PortalSecond.GetNumberOfValues();
    vtkm::Id dim12 = dim1 * dim2;
    vtkm::Id idx12 = index % dim12;

    vtkm::Id i1 = idx12 % dim1;
    vtkm::Id i2 = idx12 / dim1;
    vtkm::Id i3 = index / dim12;

    this->PortalFirst.Set(i1, value[0]);
    this->PortalSecond.Set(i2, value[1]);
    this->PortalThird.Set(i3, value[2]);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const PortalTypeFirst& GetFirstPortal() const { return this->PortalFirst; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const PortalTypeSecond& GetSecondPortal() const { return this->PortalSecond; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const PortalTypeThird& GetThirdPortal() const { return this->PortalThird; }

private:
  PortalTypeFirst PortalFirst;
  PortalTypeSecond PortalSecond;
  PortalTypeThird PortalThird;
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

template <typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
struct VTKM_ALWAYS_EXPORT StorageTagCartesianProduct
{
};

/// This helper struct defines the value type for a zip container containing
/// the given two array handles.
///
template <typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
struct ArrayHandleCartesianProductTraits
{
  /// The ValueType (a pair containing the value types of the two arrays).
  ///
  using ValueType = vtkm::Vec<typename FirstHandleType::ValueType, 3>;

  /// The appropriately templated tag.
  ///
  using Tag = StorageTagCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>;

  /// The superclass for ArrayHandleCartesianProduct.
  ///
  using Superclass = vtkm::cont::ArrayHandle<ValueType, Tag>;
};

template <typename T, typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
class Storage<T, StorageTagCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>>
{
  VTKM_IS_ARRAY_HANDLE(FirstHandleType);
  VTKM_IS_ARRAY_HANDLE(SecondHandleType);
  VTKM_IS_ARRAY_HANDLE(ThirdHandleType);

public:
  using ValueType = T;

  using PortalType =
    vtkm::exec::internal::ArrayPortalCartesianProduct<ValueType,
                                                      typename FirstHandleType::PortalControl,
                                                      typename SecondHandleType::PortalControl,
                                                      typename ThirdHandleType::PortalControl>;
  using PortalConstType =
    vtkm::exec::internal::ArrayPortalCartesianProduct<ValueType,
                                                      typename FirstHandleType::PortalConstControl,
                                                      typename SecondHandleType::PortalConstControl,
                                                      typename ThirdHandleType::PortalConstControl>;

  VTKM_CONT
  Storage()
    : FirstArray()
    , SecondArray()
    , ThirdArray()
  {
  }

  VTKM_CONT
  Storage(const FirstHandleType& array1,
          const SecondHandleType& array2,
          const ThirdHandleType& array3)
    : FirstArray(array1)
    , SecondArray(array2)
    , ThirdArray(array3)
  {
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    return PortalType(this->FirstArray.GetPortalControl(),
                      this->SecondArray.GetPortalControl(),
                      this->ThirdArray.GetPortalControl());
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->FirstArray.GetPortalConstControl(),
                           this->SecondArray.GetPortalConstControl(),
                           this->ThirdArray.GetPortalConstControl());
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->FirstArray.GetNumberOfValues() * this->SecondArray.GetNumberOfValues() *
      this->ThirdArray.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id /*numberOfValues*/)
  {
    throw vtkm::cont::ErrorBadAllocation("Does not make sense.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id /*numberOfValues*/)
  {
    throw vtkm::cont::ErrorBadAllocation("Does not make sense.");
  }

  VTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since it is asking to release the resources
    // of the arrays, which may be used elsewhere.
  }

  VTKM_CONT
  const FirstHandleType& GetFirstArray() const { return this->FirstArray; }

  VTKM_CONT
  const SecondHandleType& GetSecondArray() const { return this->SecondArray; }

  VTKM_CONT
  const ThirdHandleType& GetThirdArray() const { return this->ThirdArray; }

private:
  FirstHandleType FirstArray;
  SecondHandleType SecondArray;
  ThirdHandleType ThirdArray;
};

template <typename T,
          typename FirstHandleType,
          typename SecondHandleType,
          typename ThirdHandleType,
          typename Device>
class ArrayTransfer<T,
                    StorageTagCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>,
                    Device>
{
  using StorageTag = StorageTagCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>;
  using StorageType = vtkm::cont::internal::Storage<T, StorageTag>;

public:
  using ValueType = T;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = vtkm::exec::internal::ArrayPortalCartesianProduct<
    ValueType,
    typename FirstHandleType::template ExecutionTypes<Device>::Portal,
    typename SecondHandleType::template ExecutionTypes<Device>::Portal,
    typename ThirdHandleType::template ExecutionTypes<Device>::Portal>;

  using PortalConstExecution = vtkm::exec::internal::ArrayPortalCartesianProduct<
    ValueType,
    typename FirstHandleType::template ExecutionTypes<Device>::PortalConst,
    typename SecondHandleType::template ExecutionTypes<Device>::PortalConst,
    typename ThirdHandleType::template ExecutionTypes<Device>::PortalConst>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : FirstArray(storage->GetFirstArray())
    , SecondArray(storage->GetSecondArray())
    , ThirdArray(storage->GetThirdArray())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->FirstArray.GetNumberOfValues() * this->SecondArray.GetNumberOfValues() *
      this->ThirdArray.GetNumberOfValues();
  }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->FirstArray.PrepareForInput(Device()),
                                this->SecondArray.PrepareForInput(Device()),
                                this->ThirdArray.PrepareForInput(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    throw vtkm::cont::ErrorBadAllocation(
      "Cannot write to an ArrayHandleCartesianProduct. It does not make "
      "sense because there is overlap in the data.");
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadAllocation(
      "Cannot write to an ArrayHandleCartesianProduct. It does not make "
      "sense because there is overlap in the data.");
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // first and second array handles should automatically retrieve the
    // output data as necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id /*numberOfValues*/)
  {
    throw vtkm::cont::ErrorBadAllocation("Does not make sense.");
  }

  VTKM_CONT
  void ReleaseResources()
  {
    this->FirstArray.ReleaseResourcesExecution();
    this->SecondArray.ReleaseResourcesExecution();
    this->ThirdArray.ReleaseResourcesExecution();
  }

private:
  FirstHandleType FirstArray;
  SecondHandleType SecondArray;
  ThirdHandleType ThirdArray;
};
} // namespace internal

/// ArrayHandleCartesianProduct is a specialization of ArrayHandle. It takes two delegate
/// array handle and makes a new handle that access the corresponding entries
/// in these arrays as a pair.
///
template <typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
class ArrayHandleCartesianProduct
  : public internal::ArrayHandleCartesianProductTraits<FirstHandleType,
                                                       SecondHandleType,
                                                       ThirdHandleType>::Superclass
{
  // If the following line gives a compile error, then the FirstHandleType
  // template argument is not a valid ArrayHandle type.
  VTKM_IS_ARRAY_HANDLE(FirstHandleType);
  VTKM_IS_ARRAY_HANDLE(SecondHandleType);
  VTKM_IS_ARRAY_HANDLE(ThirdHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleCartesianProduct,
    (ArrayHandleCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>),
    (typename internal::ArrayHandleCartesianProductTraits<FirstHandleType,
                                                          SecondHandleType,
                                                          ThirdHandleType>::Superclass));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleCartesianProduct(const FirstHandleType& firstArray,
                              const SecondHandleType& secondArray,
                              const ThirdHandleType& thirdArray)
    : Superclass(StorageType(firstArray, secondArray, thirdArray))
  {
  }
};

/// A convenience function for creating an ArrayHandleCartesianProduct. It takes the two
/// arrays to be zipped together.
///
template <typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
VTKM_CONT
  vtkm::cont::ArrayHandleCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>
  make_ArrayHandleCartesianProduct(const FirstHandleType& first,
                                   const SecondHandleType& second,
                                   const ThirdHandleType& third)
{
  return ArrayHandleCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>(
    first, second, third);
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleCartesianProduct_h
