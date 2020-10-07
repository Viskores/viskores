//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_ArrayHandleCompositeVector_h
#define vtk_m_ArrayHandleCompositeVector_h

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/Deprecated.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/Tuple.h>
#include <vtkm/VecTraits.h>

#include <vtkm/internal/brigand.hpp>

#include <vtkmstd/integer_sequence.h>

#include <type_traits>

namespace vtkm
{
namespace cont
{
namespace internal
{

namespace compvec
{

// AllAreArrayHandles: ---------------------------------------------------------
// Ensures that all types in ArrayHandlesT... are subclasses of ArrayHandleBase
template <typename... ArrayHandlesT>
struct AllAreArrayHandlesImpl;

template <typename Head, typename... Tail>
struct AllAreArrayHandlesImpl<Head, Tail...>
{
private:
  using Next = AllAreArrayHandlesImpl<Tail...>;
  constexpr static bool HeadValid = std::is_base_of<ArrayHandleBase, Head>::value;

public:
  constexpr static bool Value = HeadValid && Next::Value;
};

template <typename Head>
struct AllAreArrayHandlesImpl<Head>
{
  constexpr static bool Value = std::is_base_of<ArrayHandleBase, Head>::value;
};

template <typename... ArrayHandleTs>
struct AllAreArrayHandles
{
  constexpr static bool Value = AllAreArrayHandlesImpl<ArrayHandleTs...>::Value;
};

// GetValueType: ---------------------------------------------------------------
// Determines the output `ValueType` of the set of `ArrayHandle` objects. For example, if the input
// set contains 3 types with `vtkm::Float32` ValueTypes, then the ValueType defined here will be
// `vtkm::Vec<Float32, 3>`. This also validates that all members have the same `ValueType`.

template <typename ExpectedValueType, typename ArrayType>
struct CheckValueType
{
  VTKM_STATIC_ASSERT_MSG((std::is_same<ExpectedValueType, typename ArrayType::ValueType>::value),
                         "ArrayHandleCompositeVector must be built from "
                         "ArrayHandles with the same ValueTypes.");
};

template <typename ArrayType0, typename... ArrayTypes>
struct GetValueType
{
  static constexpr vtkm::IdComponent COUNT =
    static_cast<vtkm::IdComponent>(sizeof...(ArrayTypes)) + 1;
  using ComponentType = typename ArrayType0::ValueType;
  using ValueCheck = vtkm::List<CheckValueType<ComponentType, ArrayTypes>...>;
  using ValueType = vtkm::Vec<ComponentType, COUNT>;
};

// Special case for only one component
template <typename ArrayType>
struct GetValueType<ArrayType>
{
  static constexpr vtkm::IdComponent COUNT = 1;
  using ComponentType = typename ArrayType::ValueType;
  using ValueType = typename ArrayType::ValueType;
};

// -----------------------------------------------------------------------------
// Functors to access Storage methods. This is used with vtkm::Tuple's
// ForEach and Transform methods.

struct WritePortal
{
  template <typename ArrayHandle>
  typename ArrayHandle::WritePortalType operator()(const ArrayHandle& array) const
  {
    return array.WritePortal();
  }
};

struct ReadPortal
{
  template <typename ArrayHandle>
  typename ArrayHandle::ReadPortalType operator()(const ArrayHandle& array) const
  {
    return array.ReadPortal();
  }
};

struct Allocate
{
  vtkm::Id NumValues;
  VTKM_CONT Allocate(vtkm::Id numValues)
    : NumValues(numValues)
  {
  }

  template <typename Array>
  VTKM_CONT void operator()(Array& array)
  {
    array.Allocate(this->NumValues);
  }
};

struct Shrink
{
  vtkm::Id NumValues;
  VTKM_CONT Shrink(vtkm::Id numValues)
    : NumValues(numValues)
  {
  }

  template <typename Array>
  VTKM_CONT void operator()(Array& array)
  {
    array.Shrink(this->NumValues);
  }
};

struct ReleaseResources
{
  template <typename Array>
  VTKM_CONT void operator()(Array& array)
  {
    array.ReleaseResources();
  }
};

// -----------------------------------------------------------------------------
// Functors to access ArrayTransfer methods. This is used with vtkm::Tuple's
// ForEach and Transform methods.

template <typename Device>
struct PrepareForInput
{
  vtkm::cont::Token& Token;
  VTKM_CONT PrepareForInput(vtkm::cont::Token& token)
    : Token(token)
  {
  }

  template <typename Array>
  VTKM_CONT typename Array::template ExecutionTypes<Device>::PortalConst operator()(
    const Array& array)
  {
    return array.PrepareForInput(Device{}, this->Token);
  }
};

template <typename Device>
struct PrepareForInPlace
{
  vtkm::cont::Token& Token;
  VTKM_CONT PrepareForInPlace(vtkm::cont::Token& token)
    : Token(token)
  {
  }

  template <typename Array>
  VTKM_CONT typename Array::template ExecutionTypes<Device>::Portal operator()(Array& array)
  {
    return array.PrepareForInPlace(Device{}, this->Token);
  }
};

template <typename Device>
struct PrepareForOutput
{
  vtkm::Id NumValues;
  vtkm::cont::Token& Token;
  VTKM_CONT PrepareForOutput(vtkm::Id numValues, vtkm::cont::Token& token)
    : NumValues(numValues)
    , Token(token)
  {
  }

  template <typename Array>
  VTKM_CONT typename Array::template ExecutionTypes<Device>::Portal operator()(Array& array)
  {
    return array.PrepareForOutput(this->NumValues, Device{}, this->Token);
  }
};

struct ReleaseResourcesExecution
{
  template <typename Array>
  VTKM_CONT void operator()(Array& array)
  {
    array.ReleaseResourcesExecution();
  }
};

// ArraySizeValidator: ---------------------------------------------------------
// Call Exec(ArrayTuple, NumValues) to ensure that all arrays in the tuple have
// the specified number of values.
template <std::size_t Index, std::size_t Count, typename TupleType>
struct ArraySizeValidatorImpl
{
  using Next = ArraySizeValidatorImpl<Index + 1, Count, TupleType>;

  VTKM_CONT
  static bool Exec(const TupleType& tuple, vtkm::Id numVals)
  {
    return vtkm::Get<Index>(tuple).GetNumberOfValues() == numVals && Next::Exec(tuple, numVals);
  }
};

template <std::size_t Index, typename TupleType>
struct ArraySizeValidatorImpl<Index, Index, TupleType>
{
  VTKM_CONT
  static bool Exec(const TupleType&, vtkm::Id) { return true; }
};

template <typename TupleType>
struct ArraySizeValidator
{
  VTKM_CONT
  static bool Exec(const TupleType& tuple, vtkm::Id numVals)
  {
    return ArraySizeValidatorImpl<0, vtkm::TupleSize<TupleType>::value, TupleType>::Exec(tuple,
                                                                                         numVals);
  }
};

template <typename... PortalList>
using AllPortalsAreWritable =
  typename brigand::all<brigand::list<PortalList...>,
                        brigand::bind<vtkm::internal::PortalSupportsSets, brigand::_1>>::type;

// GetFromPortals: -------------------------------------------------------------
// Given a set of array portals as arguments, returns a Vec comprising the values
// at the provided index.
VTKM_SUPPRESS_EXEC_WARNINGS
template <typename... Portals>
VTKM_EXEC_CONT typename GetValueType<Portals...>::ValueType GetFromPortals(
  vtkm::Id index,
  const Portals&... portals)
{
  return { portals.Get(index)... };
}

// SetToPortals: ---------------------------------------------------------------
// Given a Vec-like object, and index, and a set of array portals, sets each of
// the portals to the respective component of the Vec.
VTKM_SUPPRESS_EXEC_WARNINGS
template <typename ValueType, vtkm::IdComponent... I, typename... Portals>
VTKM_EXEC_CONT void SetToPortalsImpl(vtkm::Id index,
                                     const ValueType& value,
                                     vtkmstd::integer_sequence<vtkm::IdComponent, I...>,
                                     const Portals&... portals)
{
  using Traits = vtkm::VecTraits<ValueType>;
  (void)std::initializer_list<bool>{ (portals.Set(index, Traits::GetComponent(value, I)),
                                      false)... };
}

VTKM_SUPPRESS_EXEC_WARNINGS
template <typename ValueType, typename... Portals>
VTKM_EXEC_CONT void SetToPortals(vtkm::Id index, const ValueType& value, const Portals&... portals)
{
  SetToPortalsImpl(
    index,
    value,
    vtkmstd::make_integer_sequence<vtkm::IdComponent, vtkm::IdComponent(sizeof...(Portals))>{},
    portals...);
}

} // end namespace compvec

template <typename... PortalTypes>
class VTKM_ALWAYS_EXPORT ArrayPortalCompositeVector
{
  using Writable = compvec::AllPortalsAreWritable<PortalTypes...>;
  using TupleType = vtkm::Tuple<PortalTypes...>;
  TupleType Portals;

public:
  using ValueType = typename compvec::GetValueType<PortalTypes...>::ValueType;

  VTKM_EXEC_CONT
  ArrayPortalCompositeVector() {}

  VTKM_CONT
  ArrayPortalCompositeVector(const PortalTypes&... portals)
    : Portals(portals...)
  {
  }

  VTKM_CONT
  ArrayPortalCompositeVector(const TupleType& portals)
    : Portals(portals)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return vtkm::Get<0>(this->Portals).GetNumberOfValues(); }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    return this->Portals.Apply(compvec::GetFromPortals<PortalTypes...>, index);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    this->Portals.Apply(compvec::SetToPortals<ValueType, PortalTypes...>, index, value);
  }
};

} // namespace internal

template <typename... StorageTags>
struct VTKM_ALWAYS_EXPORT StorageTagCompositeVec
{
};

namespace internal
{

template <typename... ArrayTs>
struct CompositeVectorTraits
{
  // Need to check this here, since this traits struct is used in the
  // ArrayHandleCompositeVector superclass definition before any other
  // static_asserts could be used.
  VTKM_STATIC_ASSERT_MSG(compvec::AllAreArrayHandles<ArrayTs...>::Value,
                         "Template parameters for ArrayHandleCompositeVector "
                         "must be a list of ArrayHandle types.");

  using ValueType = typename compvec::GetValueType<ArrayTs...>::ValueType;
  using StorageTag = vtkm::cont::StorageTagCompositeVec<typename ArrayTs::StorageTag...>;
  using StorageType = Storage<ValueType, StorageTag>;
  using Superclass = ArrayHandle<ValueType, StorageTag>;
};

template <typename T, typename... StorageTags>
class Storage<vtkm::Vec<T, static_cast<vtkm::IdComponent>(sizeof...(StorageTags))>,
              vtkm::cont::StorageTagCompositeVec<StorageTags...>>
{
  using ArrayTuple = vtkm::Tuple<vtkm::cont::ArrayHandle<T, StorageTags>...>;

  ArrayTuple Arrays;
  bool Valid;

public:
  using ValueType = vtkm::Vec<T, static_cast<vtkm::IdComponent>(sizeof...(StorageTags))>;
  using PortalType = ArrayPortalCompositeVector<
    typename vtkm::cont::ArrayHandle<T, StorageTags>::WritePortalType...>;
  using PortalConstType =
    ArrayPortalCompositeVector<typename vtkm::cont::ArrayHandle<T, StorageTags>::ReadPortalType...>;

  VTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  template <typename... ArrayTypes>
  VTKM_CONT Storage(const ArrayTypes&... arrays)
    : Arrays(arrays...)
    , Valid(true)
  {
    using SizeValidator = compvec::ArraySizeValidator<ArrayTuple>;
    if (!SizeValidator::Exec(this->Arrays, this->GetNumberOfValues()))
    {
      throw ErrorBadValue("All arrays must have the same number of values.");
    }
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    VTKM_ASSERT(this->Valid);
    return this->Arrays.Transform(compvec::WritePortal{});
  }

  void TypeCheck(int) const;
  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Valid);
    this->Arrays.Transform(compvec::ReadPortal{});
    return this->Arrays.Transform(compvec::ReadPortal{});
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Valid);
    return vtkm::Get<0>(this->Arrays).GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id numValues)
  {
    VTKM_ASSERT(this->Valid);
    this->Arrays.ForEach(compvec::Allocate{ numValues });
  }

  VTKM_CONT
  void Shrink(vtkm::Id numValues)
  {
    VTKM_ASSERT(this->Valid);
    this->Arrays.ForEach(compvec::Shrink{ numValues });
  }

  VTKM_CONT
  void ReleaseResources()
  {
    VTKM_ASSERT(this->Valid);
    this->Arrays.ForEach(compvec::ReleaseResources{});
  }

  VTKM_CONT
  const ArrayTuple& GetArrayTuple() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Arrays;
  }

  VTKM_CONT
  ArrayTuple& GetArrayTuple()
  {
    VTKM_ASSERT(this->Valid);
    return this->Arrays;
  }
};

// Special case for single component. Just defer to the original storage.
template <typename T, typename StorageTag>
class Storage<T, vtkm::cont::StorageTagCompositeVec<StorageTag>> : public Storage<T, StorageTag>
{
  using ArrayType = vtkm::cont::ArrayHandle<T, StorageTag>;
  using TupleType = vtkm::Tuple<ArrayType>;

public:
  Storage() = default;
  Storage(const ArrayType& array)
    : Storage<T, StorageTag>(array.GetStorage())
  {
  }

  VTKM_CONT
  const TupleType GetArrayTuple() const { return TupleType(ArrayType(this->GetStoragea())); }
};

template <typename T, typename... StorageTags, typename DeviceTag>
class ArrayTransfer<vtkm::Vec<T, static_cast<vtkm::IdComponent>(sizeof...(StorageTags))>,
                    vtkm::cont::StorageTagCompositeVec<StorageTags...>,
                    DeviceTag>
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceTag);

  using ArrayTuple = vtkm::Tuple<vtkm::cont::ArrayHandle<T, StorageTags>...>;

public:
  using ValueType = vtkm::Vec<T, static_cast<vtkm::IdComponent>(sizeof...(StorageTags))>;

private:
  using StorageTag = vtkm::cont::StorageTagCompositeVec<StorageTags...>;
  using StorageType = internal::Storage<ValueType, StorageTag>;

  StorageType* Storage;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution =
    ArrayPortalCompositeVector<typename vtkm::cont::ArrayHandle<T, StorageTags>::
                                 template ExecutionTypes<DeviceTag>::Portal...>;
  using PortalConstExecution =
    ArrayPortalCompositeVector<typename vtkm::cont::ArrayHandle<T, StorageTags>::
                                 template ExecutionTypes<DeviceTag>::PortalConst...>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Storage(storage)
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData), vtkm::cont::Token& token) const
  {
    return this->GetArrayTuple().Transform(compvec::PrepareForInput<DeviceTag>{ token });
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData), vtkm::cont::Token& token)
  {
    return this->GetArrayTuple().Transform(compvec::PrepareForInPlace<DeviceTag>{ token });
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numValues, vtkm::cont::Token& token)
  {
    return this->GetArrayTuple().Transform(
      compvec::PrepareForOutput<DeviceTag>{ numValues, token });
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handle should automatically retrieve the output data as
    // necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id numValues) { this->GetArrayTuple().ForEach(compvec::Shrink{ numValues }); }

  VTKM_CONT
  void ReleaseResources() { this->GetArrayTuple().ForEach(compvec::ReleaseResourcesExecution{}); }

  VTKM_CONT
  const ArrayTuple& GetArrayTuple() const { return this->Storage->GetArrayTuple(); }
  ArrayTuple& GetArrayTuple() { return this->Storage->GetArrayTuple(); }
};

} // namespace internal

/// \brief An \c ArrayHandle that combines components from other arrays.
///
/// \c ArrayHandleCompositeVector is a specialization of \c ArrayHandle that
/// derives its content from other arrays. It takes any number of
/// single-component \c ArrayHandle objects and mimics an array that contains
/// vectors with components that come from these delegate arrays.
///
/// The easiest way to create and type an \c ArrayHandleCompositeVector is
/// to use the \c make_ArrayHandleCompositeVector functions.
///
/// The \c ArrayHandleExtractComponent class may be helpful when a desired
/// component is part of an \c ArrayHandle with a \c vtkm::Vec \c ValueType.
///
template <typename... ArrayTs>
class ArrayHandleCompositeVector
  : public ArrayHandle<typename internal::CompositeVectorTraits<ArrayTs...>::ValueType,
                       typename internal::CompositeVectorTraits<ArrayTs...>::StorageTag>
{
private:
  using Traits = internal::CompositeVectorTraits<ArrayTs...>;
  using TupleType = vtkm::Tuple<ArrayTs...>;
  using StorageType = typename Traits::StorageType;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleCompositeVector,
                             (ArrayHandleCompositeVector<ArrayTs...>),
                             (typename Traits::Superclass));

  VTKM_CONT
  ArrayHandleCompositeVector(const ArrayTs&... arrays)
    : Superclass(StorageType(arrays...))
  {
  }
};

/// Create a composite vector array from other arrays.
///
template <typename... ArrayTs>
VTKM_CONT ArrayHandleCompositeVector<ArrayTs...> make_ArrayHandleCompositeVector(
  const ArrayTs&... arrays)
{
  VTKM_STATIC_ASSERT_MSG(internal::compvec::AllAreArrayHandles<ArrayTs...>::Value,
                         "Arguments to make_ArrayHandleCompositeVector must be "
                         "of ArrayHandle types.");
  return ArrayHandleCompositeVector<ArrayTs...>(arrays...);
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

template <typename... AHs>
struct SerializableTypeString<vtkm::cont::ArrayHandleCompositeVector<AHs...>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name =
      "AH_CompositeVector<" + internal::GetVariadicSerializableTypeString(AHs{}...) + ">";
    return name;
  }
};

template <typename T, typename... STs>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<vtkm::Vec<T, static_cast<vtkm::IdComponent>(sizeof...(STs))>,
                          vtkm::cont::StorageTagCompositeVec<STs...>>>
  : SerializableTypeString<
      vtkm::cont::ArrayHandleCompositeVector<vtkm::cont::ArrayHandle<T, STs>...>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename... AHs>
struct Serialization<vtkm::cont::ArrayHandleCompositeVector<AHs...>>
{
private:
  using Type = typename vtkm::cont::ArrayHandleCompositeVector<AHs...>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

  struct SaveFunctor
  {
    BinaryBuffer& Buffer;
    SaveFunctor(BinaryBuffer& bb)
      : Buffer(bb)
    {
    }

    template <typename AH>
    void operator()(const AH& ah) const
    {
      vtkmdiy::save(this->Buffer, ah);
    }
  };

  struct LoadFunctor
  {
    BinaryBuffer& Buffer;
    LoadFunctor(BinaryBuffer& bb)
      : Buffer(bb)
    {
    }

    template <typename AH>
    void operator()(AH& ah) const
    {
      vtkmdiy::load(this->Buffer, ah);
    }
  };

  static BaseType Create(const AHs&... arrays) { return Type(arrays...); }

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    obj.GetStorage().GetArrayTuple().ForEach(SaveFunctor{ bb });
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    vtkm::Tuple<AHs...> tuple;
    tuple.ForEach(LoadFunctor{ bb });
    obj = tuple.Apply(Create);
  }
};

template <typename T, typename... STs>
struct Serialization<
  vtkm::cont::ArrayHandle<vtkm::Vec<T, static_cast<vtkm::IdComponent>(sizeof...(STs))>,
                          vtkm::cont::StorageTagCompositeVec<STs...>>>
  : Serialization<vtkm::cont::ArrayHandleCompositeVector<vtkm::cont::ArrayHandle<T, STs>...>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_ArrayHandleCompositeVector_h
