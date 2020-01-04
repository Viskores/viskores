//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

  using set_supported_p1 = vtkm::internal::PortalSupportsSets<PortalTypeFirst>;
  using set_supported_p2 = vtkm::internal::PortalSupportsSets<PortalTypeSecond>;
  using set_supported_p3 = vtkm::internal::PortalSupportsSets<PortalTypeThird>;

  using Writable = std::integral_constant<bool,
                                          set_supported_p1::value && set_supported_p2::value &&
                                            set_supported_p3::value>;

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
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
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

template <typename StorageTag1, typename StorageTag2, typename StorageTag3>
struct VTKM_ALWAYS_EXPORT StorageTagCartesianProduct
{
};

namespace internal
{

/// This helper struct defines the value type for a zip container containing
/// the given two array handles.
///
template <typename AH1, typename AH2, typename AH3>
struct ArrayHandleCartesianProductTraits
{
  VTKM_IS_ARRAY_HANDLE(AH1);
  VTKM_IS_ARRAY_HANDLE(AH2);
  VTKM_IS_ARRAY_HANDLE(AH3);

  using ComponentType = typename AH1::ValueType;
  VTKM_STATIC_ASSERT_MSG(
    (std::is_same<ComponentType, typename AH2::ValueType>::value),
    "All arrays for ArrayHandleCartesianProduct must have the same value type. "
    "Use ArrayHandleCast as necessary to make types match.");
  VTKM_STATIC_ASSERT_MSG(
    (std::is_same<ComponentType, typename AH3::ValueType>::value),
    "All arrays for ArrayHandleCartesianProduct must have the same value type. "
    "Use ArrayHandleCast as necessary to make types match.");

  /// The ValueType (a pair containing the value types of the two arrays).
  ///
  using ValueType = vtkm::Vec<ComponentType, 3>;

  /// The appropriately templated tag.
  ///
  using Tag = vtkm::cont::StorageTagCartesianProduct<typename AH1::StorageTag,
                                                     typename AH2::StorageTag,
                                                     typename AH3::StorageTag>;

  /// The superclass for ArrayHandleCartesianProduct.
  ///
  using Superclass = vtkm::cont::ArrayHandle<ValueType, Tag>;
};

template <typename T, typename ST1, typename ST2, typename ST3>
class Storage<vtkm::Vec<T, 3>, vtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>
{
  using AH1 = vtkm::cont::ArrayHandle<T, ST1>;
  using AH2 = vtkm::cont::ArrayHandle<T, ST2>;
  using AH3 = vtkm::cont::ArrayHandle<T, ST3>;

public:
  using ValueType = vtkm::Vec<typename AH1::ValueType, 3>;

  using PortalType = vtkm::exec::internal::ArrayPortalCartesianProduct<ValueType,
                                                                       typename AH1::PortalControl,
                                                                       typename AH2::PortalControl,
                                                                       typename AH3::PortalControl>;
  using PortalConstType =
    vtkm::exec::internal::ArrayPortalCartesianProduct<ValueType,
                                                      typename AH1::PortalConstControl,
                                                      typename AH2::PortalConstControl,
                                                      typename AH3::PortalConstControl>;

  VTKM_CONT
  Storage()
    : FirstArray()
    , SecondArray()
    , ThirdArray()
  {
  }

  VTKM_CONT
  Storage(const AH1& array1, const AH2& array2, const AH3& array3)
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
  const AH1& GetFirstArray() const { return this->FirstArray; }

  VTKM_CONT
  const AH2& GetSecondArray() const { return this->SecondArray; }

  VTKM_CONT
  const AH3& GetThirdArray() const { return this->ThirdArray; }

private:
  AH1 FirstArray;
  AH2 SecondArray;
  AH3 ThirdArray;
};

template <typename T, typename ST1, typename ST2, typename ST3, typename Device>
class ArrayTransfer<vtkm::Vec<T, 3>, vtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>, Device>
{
public:
  using ValueType = vtkm::Vec<T, 3>;

private:
  using AH1 = vtkm::cont::ArrayHandle<T, ST1>;
  using AH2 = vtkm::cont::ArrayHandle<T, ST2>;
  using AH3 = vtkm::cont::ArrayHandle<T, ST3>;

  using StorageTag = vtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = vtkm::exec::internal::ArrayPortalCartesianProduct<
    ValueType,
    typename AH1::template ExecutionTypes<Device>::Portal,
    typename AH2::template ExecutionTypes<Device>::Portal,
    typename AH3::template ExecutionTypes<Device>::Portal>;

  using PortalConstExecution = vtkm::exec::internal::ArrayPortalCartesianProduct<
    ValueType,
    typename AH1::template ExecutionTypes<Device>::PortalConst,
    typename AH2::template ExecutionTypes<Device>::PortalConst,
    typename AH3::template ExecutionTypes<Device>::PortalConst>;

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
  AH1 FirstArray;
  AH2 SecondArray;
  AH3 ThirdArray;
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

  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandleCartesianProduct() {}
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

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename AH1, typename AH2, typename AH3>
struct SerializableTypeString<vtkm::cont::ArrayHandleCartesianProduct<AH1, AH2, AH3>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_CartesianProduct<" + SerializableTypeString<AH1>::Get() + "," +
      SerializableTypeString<AH2>::Get() + "," + SerializableTypeString<AH3>::Get() + ">";
    return name;
  }
};

template <typename T, typename ST1, typename ST2, typename ST3>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, vtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<T, ST1>,
                                                                   vtkm::cont::ArrayHandle<T, ST2>,
                                                                   vtkm::cont::ArrayHandle<T, ST3>>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH1, typename AH2, typename AH3>
struct Serialization<vtkm::cont::ArrayHandleCartesianProduct<AH1, AH2, AH3>>
{
private:
  using Type = typename vtkm::cont::ArrayHandleCartesianProduct<AH1, AH2, AH3>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    vtkmdiy::save(bb, storage.GetFirstArray());
    vtkmdiy::save(bb, storage.GetSecondArray());
    vtkmdiy::save(bb, storage.GetThirdArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH1 array1;
    AH2 array2;
    AH3 array3;

    vtkmdiy::load(bb, array1);
    vtkmdiy::load(bb, array2);
    vtkmdiy::load(bb, array3);

    obj = vtkm::cont::make_ArrayHandleCartesianProduct(array1, array2, array3);
  }
};

template <typename T, typename ST1, typename ST2, typename ST3>
struct Serialization<
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, vtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>>
  : Serialization<vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<T, ST1>,
                                                          vtkm::cont::ArrayHandle<T, ST2>,
                                                          vtkm::cont::ArrayHandle<T, ST3>>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleCartesianProduct_h
