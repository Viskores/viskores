//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleZip_h
#define vtk_m_cont_ArrayHandleZip_h

#include <vtkm/Pair.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/internal/ArrayPortalHelpers.h>

namespace vtkm
{
namespace exec
{
namespace internal
{

/// \brief An array portal that zips two portals together into a single value
/// for the execution environment
template <typename ValueType_, typename PortalTypeFirst_, typename PortalTypeSecond_>
class ArrayPortalZip
{
public:
  using ValueType = ValueType_;
  using T = typename ValueType::FirstType;
  using U = typename ValueType::SecondType;

  using IteratorType = ValueType_;
  using PortalTypeFirst = PortalTypeFirst_;
  using PortalTypeSecond = PortalTypeSecond_;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalZip()
    : PortalFirst()
    , PortalSecond()
  {
  } //needs to be host and device so that cuda can create lvalue of these

  VTKM_CONT
  ArrayPortalZip(const PortalTypeFirst& portalfirst, const PortalTypeSecond& portalsecond)
    : PortalFirst(portalfirst)
    , PortalSecond(portalsecond)
  {
  }

  /// Copy constructor for any other ArrayPortalZip with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template <class OtherV, class OtherF, class OtherS>
  VTKM_CONT ArrayPortalZip(const ArrayPortalZip<OtherV, OtherF, OtherS>& src)
    : PortalFirst(src.GetFirstPortal())
    , PortalSecond(src.GetSecondPortal())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->PortalFirst.GetNumberOfValues(); }

  VTKM_EXEC
  ValueType Get(vtkm::Id index) const
  {
    using call_supported_t1 = typename vtkm::internal::PortalSupportsGets<PortalTypeFirst>::type;
    using call_supported_t2 = typename vtkm::internal::PortalSupportsGets<PortalTypeSecond>::type;

    return vtkm::make_Pair(this->GetFirst(call_supported_t1(), index),
                           this->GetSecond(call_supported_t2(), index));
  }

  VTKM_EXEC
  void Set(vtkm::Id index, const ValueType& value) const
  {
    using call_supported_t1 = typename vtkm::internal::PortalSupportsSets<PortalTypeFirst>::type;
    using call_supported_t2 = typename vtkm::internal::PortalSupportsSets<PortalTypeSecond>::type;
    this->SetFirst(call_supported_t1(), index, value.first);
    this->SetSecond(call_supported_t2(), index, value.second);
  }

  VTKM_EXEC_CONT
  const PortalTypeFirst& GetFirstPortal() const { return this->PortalFirst; }

  VTKM_EXEC_CONT
  const PortalTypeSecond& GetSecondPortal() const { return this->PortalSecond; }

private:
  VTKM_EXEC inline T GetFirst(std::true_type, vtkm::Id index) const noexcept
  {
    return this->PortalFirst.Get(index);
  }
  VTKM_EXEC inline T GetFirst(std::false_type, vtkm::Id) const noexcept { return T{}; }
  VTKM_EXEC inline U GetSecond(std::true_type, vtkm::Id index) const noexcept
  {
    return this->PortalSecond.Get(index);
  }
  VTKM_EXEC inline U GetSecond(std::false_type, vtkm::Id) const noexcept { return U{}; }

  VTKM_EXEC inline void SetFirst(std::true_type, vtkm::Id index, const T& value) const noexcept
  {
    this->PortalFirst.Set(index, value);
  }
  VTKM_EXEC inline void SetFirst(std::false_type, vtkm::Id, const T&) const noexcept {}
  VTKM_EXEC inline void SetSecond(std::true_type, vtkm::Id index, const U& value) const noexcept
  {
    this->PortalSecond.Set(index, value);
  }
  VTKM_EXEC inline void SetSecond(std::false_type, vtkm::Id, const U&) const noexcept {}

  PortalTypeFirst PortalFirst;
  PortalTypeSecond PortalSecond;
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

template <typename FirstHandleType, typename SecondHandleType>
struct VTKM_ALWAYS_EXPORT StorageTagZip
{
};

/// This helper struct defines the value type for a zip container containing
/// the given two array handles.
///
template <typename FirstHandleType, typename SecondHandleType>
struct ArrayHandleZipTraits
{
  /// The ValueType (a pair containing the value types of the two arrays).
  ///
  using ValueType =
    vtkm::Pair<typename FirstHandleType::ValueType, typename SecondHandleType::ValueType>;

  /// The appropriately templated tag.
  ///
  using Tag = StorageTagZip<FirstHandleType, SecondHandleType>;

  /// The superclass for ArrayHandleZip.
  ///
  using Superclass = vtkm::cont::ArrayHandle<ValueType, Tag>;
};

template <typename FirstHandleType, typename SecondHandleType>
class Storage<vtkm::Pair<typename FirstHandleType::ValueType, typename SecondHandleType::ValueType>,
              StorageTagZip<FirstHandleType, SecondHandleType>>
{
  VTKM_IS_ARRAY_HANDLE(FirstHandleType);
  VTKM_IS_ARRAY_HANDLE(SecondHandleType);

public:
  using ValueType =
    vtkm::Pair<typename FirstHandleType::ValueType, typename SecondHandleType::ValueType>;

  using PortalType = vtkm::exec::internal::ArrayPortalZip<ValueType,
                                                          typename FirstHandleType::PortalControl,
                                                          typename SecondHandleType::PortalControl>;
  using PortalConstType =
    vtkm::exec::internal::ArrayPortalZip<ValueType,
                                         typename FirstHandleType::PortalConstControl,
                                         typename SecondHandleType::PortalConstControl>;

  VTKM_CONT
  Storage()
    : FirstArray()
    , SecondArray()
  {
  }

  VTKM_CONT
  Storage(const FirstHandleType& farray, const SecondHandleType& sarray)
    : FirstArray(farray)
    , SecondArray(sarray)
  {
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    return PortalType(this->FirstArray.GetPortalControl(), this->SecondArray.GetPortalControl());
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->FirstArray.GetPortalConstControl(),
                           this->SecondArray.GetPortalConstControl());
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->FirstArray.GetNumberOfValues() == this->SecondArray.GetNumberOfValues());
    return this->FirstArray.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues)
  {
    this->FirstArray.Allocate(numberOfValues);
    this->SecondArray.Allocate(numberOfValues);
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    this->FirstArray.Shrink(numberOfValues);
    this->SecondArray.Shrink(numberOfValues);
  }

  VTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since it is asking to release the resources
    // of the two zipped array, which may be used elsewhere.
  }

  VTKM_CONT
  const FirstHandleType& GetFirstArray() const { return this->FirstArray; }

  VTKM_CONT
  const SecondHandleType& GetSecondArray() const { return this->SecondArray; }

private:
  FirstHandleType FirstArray;
  SecondHandleType SecondArray;
};

template <typename FirstHandleType, typename SecondHandleType, typename Device>
class ArrayTransfer<
  vtkm::Pair<typename FirstHandleType::ValueType, typename SecondHandleType::ValueType>,
  StorageTagZip<FirstHandleType, SecondHandleType>,
  Device>
{
public:
  using ValueType =
    vtkm::Pair<typename FirstHandleType::ValueType, typename SecondHandleType::ValueType>;

private:
  using StorageTag = StorageTagZip<FirstHandleType, SecondHandleType>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = vtkm::exec::internal::ArrayPortalZip<
    ValueType,
    typename FirstHandleType::template ExecutionTypes<Device>::Portal,
    typename SecondHandleType::template ExecutionTypes<Device>::Portal>;

  using PortalConstExecution = vtkm::exec::internal::ArrayPortalZip<
    ValueType,
    typename FirstHandleType::template ExecutionTypes<Device>::PortalConst,
    typename SecondHandleType::template ExecutionTypes<Device>::PortalConst>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : FirstArray(storage->GetFirstArray())
    , SecondArray(storage->GetSecondArray())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->FirstArray.GetNumberOfValues() == this->SecondArray.GetNumberOfValues());
    return this->FirstArray.GetNumberOfValues();
  }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->FirstArray.PrepareForInput(Device()),
                                this->SecondArray.PrepareForInput(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return PortalExecution(this->FirstArray.PrepareForInPlace(Device()),
                           this->SecondArray.PrepareForInPlace(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return PortalExecution(this->FirstArray.PrepareForOutput(numberOfValues, Device()),
                           this->SecondArray.PrepareForOutput(numberOfValues, Device()));
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // first and second array handles should automatically retrieve the
    // output data as necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    this->FirstArray.Shrink(numberOfValues);
    this->SecondArray.Shrink(numberOfValues);
  }

  VTKM_CONT
  void ReleaseResources()
  {
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
template <typename FirstHandleType, typename SecondHandleType>
class ArrayHandleZip
  : public internal::ArrayHandleZipTraits<FirstHandleType, SecondHandleType>::Superclass
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
    (ArrayHandleZip<FirstHandleType, SecondHandleType>),
    (typename internal::ArrayHandleZipTraits<FirstHandleType, SecondHandleType>::Superclass));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleZip(const FirstHandleType& firstArray, const SecondHandleType& secondArray)
    : Superclass(StorageType(firstArray, secondArray))
  {
  }
};

/// A convenience function for creating an ArrayHandleZip. It takes the two
/// arrays to be zipped together.
///
template <typename FirstHandleType, typename SecondHandleType>
VTKM_CONT vtkm::cont::ArrayHandleZip<FirstHandleType, SecondHandleType> make_ArrayHandleZip(
  const FirstHandleType& first,
  const SecondHandleType& second)
{
  return ArrayHandleZip<FirstHandleType, SecondHandleType>(first, second);
}
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
namespace vtkm
{
namespace cont
{

template <typename AH1, typename AH2>
struct SerializableTypeString<vtkm::cont::ArrayHandleZip<AH1, AH2>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Zip<" + SerializableTypeString<AH1>::Get() + "," +
      SerializableTypeString<AH2>::Get() + ">";
    return name;
  }
};

template <typename AH1, typename AH2>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<vtkm::Pair<typename AH1::ValueType, typename AH2::ValueType>,
                          vtkm::cont::internal::StorageTagZip<AH1, AH2>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleZip<AH1, AH2>>
{
};
}
} // namespace vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH1, typename AH2>
struct Serialization<vtkm::cont::ArrayHandleZip<AH1, AH2>>
{
private:
  using Type = typename vtkm::cont::ArrayHandleZip<AH1, AH2>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    vtkmdiy::save(bb, storage.GetFirstArray());
    vtkmdiy::save(bb, storage.GetSecondArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH1 a1;
    AH2 a2;

    vtkmdiy::load(bb, a1);
    vtkmdiy::load(bb, a2);

    obj = vtkm::cont::make_ArrayHandleZip(a1, a2);
  }
};

template <typename AH1, typename AH2>
struct Serialization<
  vtkm::cont::ArrayHandle<vtkm::Pair<typename AH1::ValueType, typename AH2::ValueType>,
                          vtkm::cont::internal::StorageTagZip<AH1, AH2>>>
  : Serialization<vtkm::cont::ArrayHandleZip<AH1, AH2>>
{
};

} // diy

#endif //vtk_m_cont_ArrayHandleZip_h
