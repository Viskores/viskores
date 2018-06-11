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
#ifndef vtk_m_ArrayHandleCompositeVector_h
#define vtk_m_ArrayHandleCompositeVector_h

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/StaticAssert.h>
#include <vtkm/VecTraits.h>

#include <vtkmtaotuple/include/Tuple.h>

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

// ParamsAreArrayHandles: ------------------------------------------------------
// Same as AllAreArrayHandles, but accepts a tuple.
template <typename T>
struct ParamsAreArrayHandles
{
  constexpr static bool Value = false;
};

template <typename... ArrayHandleTs>
struct ParamsAreArrayHandles<vtkmstd::tuple<ArrayHandleTs...>>
{
  constexpr static bool Value = AllAreArrayHandlesImpl<ArrayHandleTs...>::Value;
};

// GetValueType: ---------------------------------------------------------------
// Determines the output ValueType of the objects in TupleType, a vtkmstd::tuple
// which can contain ArrayHandles, ArrayPortals...anything with a ValueType
// defined, really. For example, if the input TupleType contains 3 types with
// Float32 ValueTypes, then the ValueType defined here will be Vec<Float32, 3>.
// This also validates that all members have the same ValueType.
template <typename TupleType>
struct GetValueTypeImpl;

template <typename Head, typename... Tail>
struct GetValueTypeImpl<vtkmstd::tuple<Head, Tail...>>
{
  using Type = typename Head::ValueType;

private:
  using Next = GetValueTypeImpl<vtkmstd::tuple<Tail...>>;
  VTKM_STATIC_ASSERT_MSG(VTKM_PASS_COMMAS(std::is_same<Type, typename Next::Type>::value),
                         "ArrayHandleCompositeVector must be built from "
                         "ArrayHandles with the same ValueTypes.");
};

template <typename Head>
struct GetValueTypeImpl<vtkmstd::tuple<Head>>
{
  using Type = typename Head::ValueType;
};

template <typename TupleType>
struct GetValueType
{
  VTKM_STATIC_ASSERT(vtkmstd::tuple_size<TupleType>::value >= 1);

  static const vtkm::IdComponent COUNT =
    static_cast<vtkm::IdComponent>(vtkmstd::tuple_size<TupleType>::value);
  using ComponentType = typename GetValueTypeImpl<TupleType>::Type;
  using ValueType = vtkm::Vec<ComponentType, COUNT>;
};

// TupleTypePrepend: -----------------------------------------------------------
// Prepend a type to a tuple, defining the new tuple in Type.
template <typename PrependType, typename TupleType>
struct TupleTypePrepend;

template <typename PrependType, typename... TupleTypes>
struct TupleTypePrepend<PrependType, vtkmstd::tuple<TupleTypes...>>
{
  using Type = vtkmstd::tuple<PrependType, TupleTypes...>;
};

// ArrayTupleForEach: ----------------------------------------------------------
// Collection of methods that iterate through the arrays in ArrayTuple to
// implement the ArrayHandle API.
template <std::size_t Index, std::size_t Count, typename ArrayTuple>
struct ArrayTupleForEach
{
  using Next = ArrayTupleForEach<Index + 1, Count, ArrayTuple>;

  template <typename PortalTuple>
  VTKM_CONT static void GetPortalTupleControl(ArrayTuple& arrays, PortalTuple& portals)
  {
    vtkmstd::get<Index>(portals) = vtkmstd::get<Index>(arrays).GetPortalControl();
    Next::GetPortalTupleControl(arrays, portals);
  }

  template <typename PortalTuple>
  VTKM_CONT static void GetPortalConstTupleControl(const ArrayTuple& arrays, PortalTuple& portals)
  {
    vtkmstd::get<Index>(portals) = vtkmstd::get<Index>(arrays).GetPortalConstControl();
    Next::GetPortalConstTupleControl(arrays, portals);
  }

  template <typename DeviceTag, typename PortalTuple>
  VTKM_CONT static void PrepareForInput(const ArrayTuple& arrays, PortalTuple& portals)
  {
    vtkmstd::get<Index>(portals) = vtkmstd::get<Index>(arrays).PrepareForInput(DeviceTag());
    Next::template PrepareForInput<DeviceTag>(arrays, portals);
  }

  template <typename DeviceTag, typename PortalTuple>
  VTKM_CONT static void PrepareForInPlace(ArrayTuple& arrays, PortalTuple& portals)
  {
    vtkmstd::get<Index>(portals) = vtkmstd::get<Index>(arrays).PrepareForInPlace(DeviceTag());
    Next::template PrepareForInPlace<DeviceTag>(arrays, portals);
  }

  template <typename DeviceTag, typename PortalTuple>
  VTKM_CONT static void PrepareForOutput(ArrayTuple& arrays,
                                         PortalTuple& portals,
                                         vtkm::Id numValues)
  {
    vtkmstd::get<Index>(portals) =
      vtkmstd::get<Index>(arrays).PrepareForOutput(numValues, DeviceTag());
    Next::template PrepareForOutput<DeviceTag>(arrays, portals, numValues);
  }

  VTKM_CONT
  static void Allocate(ArrayTuple& arrays, vtkm::Id numValues)
  {
    vtkmstd::get<Index>(arrays).Allocate(numValues);
    Next::Allocate(arrays, numValues);
  }

  VTKM_CONT
  static void Shrink(ArrayTuple& arrays, vtkm::Id numValues)
  {
    vtkmstd::get<Index>(arrays).Shrink(numValues);
    Next::Shrink(arrays, numValues);
  }

  VTKM_CONT
  static void ReleaseResources(ArrayTuple& arrays)
  {
    vtkmstd::get<Index>(arrays).ReleaseResources();
    Next::ReleaseResources(arrays);
  }
};

template <std::size_t Index, typename ArrayTuple>
struct ArrayTupleForEach<Index, Index, ArrayTuple>
{
  template <typename PortalTuple>
  VTKM_CONT static void GetPortalTupleControl(ArrayTuple&, PortalTuple&)
  {
  }

  template <typename PortalTuple>
  VTKM_CONT static void GetPortalConstTupleControl(const ArrayTuple&, PortalTuple&)
  {
  }

  template <typename DeviceTag, typename PortalTuple>
  VTKM_CONT static void PrepareForInput(const ArrayTuple&, PortalTuple&)
  {
  }

  template <typename DeviceTag, typename PortalTuple>
  VTKM_CONT static void PrepareForInPlace(ArrayTuple&, PortalTuple&)
  {
  }

  template <typename DeviceTag, typename PortalTuple>
  VTKM_CONT static void PrepareForOutput(ArrayTuple&, PortalTuple&, vtkm::Id)
  {
  }

  VTKM_CONT static void Allocate(ArrayTuple&, vtkm::Id) {}

  VTKM_CONT static void Shrink(ArrayTuple&, vtkm::Id) {}

  VTKM_CONT static void ReleaseResources(ArrayTuple&) {}
};

// PortalTupleTraits: ----------------------------------------------------------
// Determine types of ArrayHandleCompositeVector portals and construct the
// portals from the input arrays.
template <typename ArrayTuple>
struct PortalTupleTypeGeneratorImpl;

template <typename Head, typename... Tail>
struct PortalTupleTypeGeneratorImpl<vtkmstd::tuple<Head, Tail...>>
{
  using Next = PortalTupleTypeGeneratorImpl<vtkmstd::tuple<Tail...>>;
  using PortalControlTuple = typename TupleTypePrepend<typename Head::PortalControl,
                                                       typename Next::PortalControlTuple>::Type;
  using PortalConstControlTuple =
    typename TupleTypePrepend<typename Head::PortalConstControl,
                              typename Next::PortalConstControlTuple>::Type;

  template <typename DeviceTag>
  struct ExecutionTypes
  {
    using PortalTuple = typename TupleTypePrepend<
      typename Head::template ExecutionTypes<DeviceTag>::Portal,
      typename Next::template ExecutionTypes<DeviceTag>::PortalTuple>::Type;
    using PortalConstTuple = typename TupleTypePrepend<
      typename Head::template ExecutionTypes<DeviceTag>::PortalConst,
      typename Next::template ExecutionTypes<DeviceTag>::PortalConstTuple>::Type;
  };
};

template <typename Head>
struct PortalTupleTypeGeneratorImpl<vtkmstd::tuple<Head>>
{
  using PortalControlTuple = vtkmstd::tuple<typename Head::PortalControl>;
  using PortalConstControlTuple = vtkmstd::tuple<typename Head::PortalConstControl>;

  template <typename DeviceTag>
  struct ExecutionTypes
  {
    using PortalTuple = vtkmstd::tuple<typename Head::template ExecutionTypes<DeviceTag>::Portal>;
    using PortalConstTuple =
      vtkmstd::tuple<typename Head::template ExecutionTypes<DeviceTag>::PortalConst>;
  };
};

template <typename ArrayTuple>
struct PortalTupleTraits
{
private:
  using TypeGenerator = PortalTupleTypeGeneratorImpl<ArrayTuple>;
  using ForEachArray = ArrayTupleForEach<0, vtkmstd::tuple_size<ArrayTuple>::value, ArrayTuple>;

public:
  using PortalTuple = typename TypeGenerator::PortalControlTuple;
  using PortalConstTuple = typename TypeGenerator::PortalConstControlTuple;

  VTKM_STATIC_ASSERT(vtkmstd::tuple_size<ArrayTuple>::value ==
                     vtkmstd::tuple_size<PortalTuple>::value);
  VTKM_STATIC_ASSERT(vtkmstd::tuple_size<ArrayTuple>::value ==
                     vtkmstd::tuple_size<PortalConstTuple>::value);

  template <typename DeviceTag>
  struct ExecutionTypes
  {
    using PortalTuple = typename TypeGenerator::template ExecutionTypes<DeviceTag>::PortalTuple;
    using PortalConstTuple =
      typename TypeGenerator::template ExecutionTypes<DeviceTag>::PortalConstTuple;

    VTKM_STATIC_ASSERT(vtkmstd::tuple_size<ArrayTuple>::value ==
                       vtkmstd::tuple_size<PortalTuple>::value);
    VTKM_STATIC_ASSERT(vtkmstd::tuple_size<ArrayTuple>::value ==
                       vtkmstd::tuple_size<PortalConstTuple>::value);
  };

  VTKM_CONT
  static const PortalTuple GetPortalTupleControl(ArrayTuple& arrays)
  {
    PortalTuple portals;
    ForEachArray::GetPortalTupleControl(arrays, portals);
    return portals;
  }

  VTKM_CONT
  static const PortalConstTuple GetPortalConstTupleControl(const ArrayTuple& arrays)
  {
    PortalConstTuple portals;
    ForEachArray::GetPortalConstTupleControl(arrays, portals);
    return portals;
  }

  template <typename DeviceTag>
  VTKM_CONT static const typename ExecutionTypes<DeviceTag>::PortalConstTuple PrepareForInput(
    const ArrayTuple& arrays)
  {
    typename ExecutionTypes<DeviceTag>::PortalConstTuple portals;
    ForEachArray::template PrepareForInput<DeviceTag>(arrays, portals);
    return portals;
  }

  template <typename DeviceTag>
  VTKM_CONT static const typename ExecutionTypes<DeviceTag>::PortalTuple PrepareForInPlace(
    ArrayTuple& arrays)
  {
    typename ExecutionTypes<DeviceTag>::PortalTuple portals;
    ForEachArray::template PrepareForInPlace<DeviceTag>(arrays, portals);
    return portals;
  }

  template <typename DeviceTag>
  VTKM_CONT static const typename ExecutionTypes<DeviceTag>::PortalTuple PrepareForOutput(
    ArrayTuple& arrays,
    vtkm::Id numValues)
  {
    typename ExecutionTypes<DeviceTag>::PortalTuple portals;
    ForEachArray::template PrepareForOutput<DeviceTag>(arrays, portals, numValues);
    return portals;
  }
};

// ArraySizeValidator: ---------------------------------------------------------
// Call Exec(ArrayTuple, NumValues) to ensure that all arrays in the tuple have
// the specified number of values.
template <std::size_t Index, std::size_t Count, typename TupleType>
struct ArraySizeValidatorImpl
{
  using Next = ArraySizeValidatorImpl<Index + 1, Count, TupleType>;

  VTKM_EXEC_CONT
  static bool Exec(const TupleType& tuple, vtkm::Id numVals)
  {
    return vtkmstd::get<Index>(tuple).GetNumberOfValues() == numVals && Next::Exec(tuple, numVals);
  }
};

template <std::size_t Index, typename TupleType>
struct ArraySizeValidatorImpl<Index, Index, TupleType>
{
  VTKM_EXEC_CONT
  static bool Exec(const TupleType&, vtkm::Id) { return true; }
};

template <typename TupleType>
struct ArraySizeValidator
{
  VTKM_EXEC_CONT
  static bool Exec(const TupleType& tuple, vtkm::Id numVals)
  {
    return ArraySizeValidatorImpl<0, vtkmstd::tuple_size<TupleType>::value, TupleType>::Exec(
      tuple, numVals);
  }
};

} // end namespace compvec

template <typename PortalTuple>
class VTKM_ALWAYS_EXPORT ArrayPortalCompositeVector
{
public:
  using ValueType = typename compvec::GetValueType<PortalTuple>::ValueType;

private:
  using Traits = vtkm::VecTraits<ValueType>;

  // Get: ----------------------------------------------------------------------
  template <vtkm::IdComponent VectorIndex, typename PortalTupleT>
  struct GetImpl;

  template <vtkm::IdComponent VectorIndex, typename Head, typename... Tail>
  struct GetImpl<VectorIndex, vtkmstd::tuple<Head, Tail...>>
  {
    using Next = GetImpl<VectorIndex + 1, vtkmstd::tuple<Tail...>>;

    VTKM_EXEC_CONT
    static void Exec(const PortalTuple& portals, ValueType& vec, vtkm::Id arrayIndex)
    {
      Traits::SetComponent(vec, VectorIndex, vtkmstd::get<VectorIndex>(portals).Get(arrayIndex));
      Next::Exec(portals, vec, arrayIndex);
    }
  };

  template <vtkm::IdComponent VectorIndex, typename Head>
  struct GetImpl<VectorIndex, vtkmstd::tuple<Head>>
  {
    VTKM_EXEC_CONT
    static void Exec(const PortalTuple& portals, ValueType& vec, vtkm::Id arrayIndex)
    {
      Traits::SetComponent(vec, VectorIndex, vtkmstd::get<VectorIndex>(portals).Get(arrayIndex));
    }
  };

  // Set: ----------------------------------------------------------------------
  template <vtkm::IdComponent VectorIndex, typename PortalTupleT>
  struct SetImpl;

  template <vtkm::IdComponent VectorIndex, typename Head, typename... Tail>
  struct SetImpl<VectorIndex, vtkmstd::tuple<Head, Tail...>>
  {
    using Next = SetImpl<VectorIndex + 1, vtkmstd::tuple<Tail...>>;

    VTKM_EXEC_CONT
    static void Exec(const PortalTuple& portals, const ValueType& vec, vtkm::Id arrayIndex)
    {
      vtkmstd::get<VectorIndex>(portals).Set(arrayIndex, Traits::GetComponent(vec, VectorIndex));
      Next::Exec(portals, vec, arrayIndex);
    }
  };

  template <vtkm::IdComponent VectorIndex, typename Head>
  struct SetImpl<VectorIndex, vtkmstd::tuple<Head>>
  {
    VTKM_EXEC_CONT
    static void Exec(const PortalTuple& portals, const ValueType& vec, vtkm::Id arrayIndex)
    {
      vtkmstd::get<VectorIndex>(portals).Set(arrayIndex, Traits::GetComponent(vec, VectorIndex));
    }
  };

public:
  VTKM_EXEC_CONT
  ArrayPortalCompositeVector() {}

  VTKM_CONT
  ArrayPortalCompositeVector(const PortalTuple& portals)
    : Portals(portals)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return vtkmstd::get<0>(this->Portals).GetNumberOfValues(); }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    ValueType result;
    GetImpl<0, PortalTuple>::Exec(this->Portals, result, index);
    return result;
  }

  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const
  {
    SetImpl<0, PortalTuple>::Exec(this->Portals, value, index);
  }

private:
  PortalTuple Portals;
};

template <typename ArrayTuple>
struct VTKM_ALWAYS_EXPORT StorageTagCompositeVector
{
};

template <typename ArrayTuple>
class Storage<typename compvec::GetValueType<ArrayTuple>::ValueType,
              StorageTagCompositeVector<ArrayTuple>>
{
  using ForEachArray =
    compvec::ArrayTupleForEach<0, vtkmstd::tuple_size<ArrayTuple>::value, ArrayTuple>;
  using PortalTypes = compvec::PortalTupleTraits<ArrayTuple>;
  using PortalTupleType = typename PortalTypes::PortalTuple;
  using PortalConstTupleType = typename PortalTypes::PortalConstTuple;

public:
  using ValueType = typename compvec::GetValueType<ArrayTuple>::ValueType;
  using PortalType = ArrayPortalCompositeVector<PortalTupleType>;
  using PortalConstType = ArrayPortalCompositeVector<PortalConstTupleType>;

  VTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  VTKM_CONT
  Storage(const ArrayTuple& arrays)
    : Arrays(arrays)
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
    return PortalType(PortalTypes::GetPortalTupleControl(this->Arrays));
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Valid);
    return PortalConstType(PortalTypes::GetPortalConstTupleControl(this->Arrays));
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Valid);
    return vtkmstd::get<0>(this->Arrays).GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id numValues)
  {
    VTKM_ASSERT(this->Valid);
    return ForEachArray::Allocate(this->Arrays, numValues);
  }

  VTKM_CONT
  void Shrink(vtkm::Id numValues)
  {
    VTKM_ASSERT(this->Valid);
    return ForEachArray::Shrink(this->Arrays, numValues);
  }

  VTKM_CONT
  void ReleaseResources()
  {
    VTKM_ASSERT(this->Valid);
    return ForEachArray::ReleaseResources(this->Arrays);
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

private:
  ArrayTuple Arrays;
  bool Valid;
};

template <typename ArrayTuple, typename DeviceTag>
class ArrayTransfer<typename compvec::GetValueType<ArrayTuple>::ValueType,
                    StorageTagCompositeVector<ArrayTuple>,
                    DeviceTag>
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceTag);

public:
  using ValueType = typename compvec::GetValueType<ArrayTuple>::ValueType;

private:
  using ForEachArray =
    compvec::ArrayTupleForEach<0, vtkmstd::tuple_size<ArrayTuple>::value, ArrayTuple>;
  using StorageTag = StorageTagCompositeVector<ArrayTuple>;
  using StorageType = internal::Storage<ValueType, StorageTag>;
  using ControlTraits = compvec::PortalTupleTraits<ArrayTuple>;
  using ExecutionTraits = typename ControlTraits::template ExecutionTypes<DeviceTag>;

public:
  using PortalControl = ArrayPortalCompositeVector<typename ControlTraits::PortalTuple>;
  using PortalConstControl = ArrayPortalCompositeVector<typename ControlTraits::PortalConstTuple>;

  using PortalExecution = ArrayPortalCompositeVector<typename ExecutionTraits::PortalTuple>;
  using PortalConstExecution =
    ArrayPortalCompositeVector<typename ExecutionTraits::PortalConstTuple>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Storage(storage)
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData)) const
  {
    return PortalConstExecution(
      ControlTraits::template PrepareForInput<DeviceTag>(this->GetArrayTuple()));
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return PortalExecution(
      ControlTraits::template PrepareForInPlace<DeviceTag>(this->GetArrayTuple()));
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numValues)
  {
    return PortalExecution(
      ControlTraits::template PrepareForOutput<DeviceTag>(this->GetArrayTuple(), numValues));
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handle should automatically retrieve the output data as
    // necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id numValues) { ForEachArray::Shrink(this->GetArrayTuple(), numValues); }

  VTKM_CONT
  void ReleaseResources() { ForEachArray::ReleaseResources(this->GetArrayTuple()); }

  VTKM_CONT
  const ArrayTuple& GetArrayTuple() const { return this->Storage->GetArrayTuple(); }
  ArrayTuple& GetArrayTuple() { return this->Storage->GetArrayTuple(); }

private:
  StorageType* Storage;
};

template <typename... ArrayTs>
struct CompositeVectorTraits
{
  // Need to check this here, since this traits struct is used in the
  // ArrayHandleCompositeVector superclass definition before any other
  // static_asserts could be used.
  VTKM_STATIC_ASSERT_MSG(compvec::AllAreArrayHandles<ArrayTs...>::Value,
                         "Template parameters for ArrayHandleCompositeVector "
                         "must be a list of ArrayHandle types.");

  using ValueType = typename compvec::GetValueType<vtkmstd::tuple<ArrayTs...>>::ValueType;
  using StorageTag = StorageTagCompositeVector<vtkmstd::tuple<ArrayTs...>>;
  using StorageType = Storage<ValueType, StorageTag>;
  using Superclass = ArrayHandle<ValueType, StorageTag>;
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
  using TupleType = vtkmstd::tuple<ArrayTs...>;
  using StorageType = typename Traits::StorageType;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleCompositeVector,
                             (ArrayHandleCompositeVector<ArrayTs...>),
                             (typename Traits::Superclass));

  VTKM_CONT
  ArrayHandleCompositeVector(const ArrayTs&... arrays)
    : Superclass(StorageType(vtkmstd::make_tuple(arrays...)))
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

#endif //vtk_m_ArrayHandleCompositeVector_h
