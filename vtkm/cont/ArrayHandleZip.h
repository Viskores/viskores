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
  using ReadableP1 = vtkm::internal::PortalSupportsGets<PortalTypeFirst_>;
  using ReadableP2 = vtkm::internal::PortalSupportsGets<PortalTypeSecond_>;
  using WritableP1 = vtkm::internal::PortalSupportsSets<PortalTypeFirst_>;
  using WritableP2 = vtkm::internal::PortalSupportsSets<PortalTypeSecond_>;

  using Readable = std::integral_constant<bool, ReadableP1::value && ReadableP2::value>;
  using Writable = std::integral_constant<bool, WritableP1::value && WritableP2::value>;

public:
  using ValueType = ValueType_;
  using T = typename ValueType::FirstType;
  using U = typename ValueType::SecondType;

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

  template <typename Readable_ = Readable,
            typename = typename std::enable_if<Readable_::value>::type>
  VTKM_EXEC_CONT ValueType Get(vtkm::Id index) const noexcept
  {
    return vtkm::make_Pair(this->PortalFirst.Get(index), this->PortalSecond.Get(index));
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const noexcept
  {
    this->PortalFirst.Set(index, value.first);
    this->PortalSecond.Set(index, value.second);
  }

  VTKM_EXEC_CONT
  const PortalTypeFirst& GetFirstPortal() const { return this->PortalFirst; }

  VTKM_EXEC_CONT
  const PortalTypeSecond& GetSecondPortal() const { return this->PortalSecond; }

private:
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

template <typename ST1, typename ST2>
struct VTKM_ALWAYS_EXPORT StorageTagZip
{
};

namespace internal
{

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
  using Tag =
    StorageTagZip<typename FirstHandleType::StorageTag, typename SecondHandleType::StorageTag>;

  /// The superclass for ArrayHandleZip.
  ///
  using Superclass = vtkm::cont::ArrayHandle<ValueType, Tag>;
};

template <typename T1, typename T2, typename ST1, typename ST2>
class Storage<vtkm::Pair<T1, T2>, vtkm::cont::StorageTagZip<ST1, ST2>>
{
  using FirstHandleType = vtkm::cont::ArrayHandle<T1, ST1>;
  using SecondHandleType = vtkm::cont::ArrayHandle<T2, ST2>;

public:
  using ValueType = vtkm::Pair<T1, T2>;

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

template <typename T1, typename T2, typename ST1, typename ST2, typename Device>
class ArrayTransfer<vtkm::Pair<T1, T2>, vtkm::cont::StorageTagZip<ST1, ST2>, Device>
{
  using StorageTag = vtkm::cont::StorageTagZip<ST1, ST2>;
  using StorageType = vtkm::cont::internal::Storage<vtkm::Pair<T1, T2>, StorageTag>;

  using FirstHandleType = vtkm::cont::ArrayHandle<T1, ST1>;
  using SecondHandleType = vtkm::cont::ArrayHandle<T2, ST2>;

public:
  using ValueType = vtkm::Pair<T1, T2>;

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
/// @cond SERIALIZATION
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

template <typename T1, typename T2, typename ST1, typename ST2>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<vtkm::Pair<T1, T2>, vtkm::cont::StorageTagZip<ST1, ST2>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleZip<vtkm::cont::ArrayHandle<T1, ST1>,
                                                      vtkm::cont::ArrayHandle<T2, ST2>>>
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

template <typename T1, typename T2, typename ST1, typename ST2>
struct Serialization<
  vtkm::cont::ArrayHandle<vtkm::Pair<T1, T2>, vtkm::cont::StorageTagZip<ST1, ST2>>>
  : Serialization<vtkm::cont::ArrayHandleZip<vtkm::cont::ArrayHandle<T1, ST1>,
                                             vtkm::cont::ArrayHandle<T2, ST2>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleZip_h
