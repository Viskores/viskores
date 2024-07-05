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

#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/StaticAssert.h>
#include <vtkm/Tuple.h>
#include <vtkm/VecTraits.h>

#include <vtkmstd/integer_sequence.h>

#include <numeric>
#include <type_traits>

namespace vtkm
{
namespace internal
{

namespace compvec
{

template <typename... PortalList>
using AllPortalsAreWritable =
  vtkm::ListAll<vtkm::List<PortalList...>, vtkm::internal::PortalSupportsSets>;

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

} // namespace compvec

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
    auto getFromPortals = [index](const auto&... portals) {
      return ValueType{ portals.Get(index)... };
    };
    return this->Portals.Apply(getFromPortals);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    // Note that we are using a lambda function here to implicitly construct a
    // functor to pass to Apply. Some device compilers will not allow passing a
    // function or function pointer to Tuple::Apply.
    auto setToPortal = [index, &value](const auto&... portals) {
      compvec::SetToPortals(index, value, portals...);
    };
    this->Portals.Apply(setToPortal);
  }
};

}
} // vtkm::internal

namespace vtkm
{
namespace cont
{
namespace internal
{

namespace compvec
{

template <typename ArrayType>
struct VerifyArrayHandle
{
  VTKM_STATIC_ASSERT_MSG(vtkm::cont::internal::ArrayHandleCheck<ArrayType>::type::value,
                         "Template parameters for ArrayHandleCompositeVector "
                         "must be a list of ArrayHandle types.");
};

} // end namespace compvec

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
  using CheckArrayHandles = vtkm::List<compvec::VerifyArrayHandle<ArrayTs>...>;

  using ValueType = typename vtkm::internal::compvec::GetValueType<ArrayTs...>::ValueType;
  using StorageTag = vtkm::cont::StorageTagCompositeVec<typename ArrayTs::StorageTag...>;
  using Superclass = ArrayHandle<ValueType, StorageTag>;
};

template <typename T, typename... StorageTags>
class Storage<vtkm::Vec<T, static_cast<vtkm::IdComponent>(sizeof...(StorageTags))>,
              vtkm::cont::StorageTagCompositeVec<StorageTags...>>
{
  using ValueType = vtkm::Vec<T, static_cast<vtkm::IdComponent>(sizeof...(StorageTags))>;

  struct Info
  {
    std::array<std::size_t, sizeof...(StorageTags) + 1> BufferOffset;
  };

  template <typename S>
  using StorageFor = vtkm::cont::internal::Storage<T, S>;

  using StorageTuple = vtkm::Tuple<StorageFor<StorageTags>...>;

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> GetBuffers(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    std::size_t subArray)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<vtkm::cont::internal::Buffer>(buffers.begin() + info.BufferOffset[subArray],
                                                     buffers.begin() +
                                                       info.BufferOffset[subArray + 1]);
  }

  template <std::size_t I>
  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> Buffers(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return GetBuffers(buffers, I);
  }

  using IndexList = vtkmstd::make_index_sequence<sizeof...(StorageTags)>;

public:
  using ReadPortalType =
    vtkm::internal::ArrayPortalCompositeVector<typename StorageFor<StorageTags>::ReadPortalType...>;
  using WritePortalType = vtkm::internal::ArrayPortalCompositeVector<
    typename StorageFor<StorageTags>::WritePortalType...>;

private:
  template <std::size_t... Is>
  static void ResizeBuffersImpl(vtkmstd::index_sequence<Is...>,
                                vtkm::Id numValues,
                                const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                vtkm::CopyFlag preserve,
                                vtkm::cont::Token& token)
  {
    std::vector<std::vector<vtkm::cont::internal::Buffer>> bufferPartitions = { Buffers<Is>(
      buffers)... };
    auto init_list = { (vtkm::tuple_element_t<Is, StorageTuple>::ResizeBuffers(
                          numValues, bufferPartitions[Is], preserve, token),
                        false)... };
    (void)init_list;
  }

  template <std::size_t... Is>
  static void FillImpl(vtkmstd::index_sequence<Is...>,
                       const std::vector<vtkm::cont::internal::Buffer>& buffers,
                       const ValueType& fillValue,
                       vtkm::Id startIndex,
                       vtkm::Id endIndex,
                       vtkm::cont::Token& token)
  {
    auto init_list = { (
      vtkm::tuple_element_t<Is, StorageTuple>::Fill(Buffers<Is>(buffers),
                                                    fillValue[static_cast<vtkm::IdComponent>(Is)],
                                                    startIndex,
                                                    endIndex,
                                                    token),
      false)... };
    (void)init_list;
  }

  template <std::size_t... Is>
  static ReadPortalType CreateReadPortalImpl(
    vtkmstd::index_sequence<Is...>,
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return ReadPortalType(vtkm::tuple_element_t<Is, StorageTuple>::CreateReadPortal(
      Buffers<Is>(buffers), device, token)...);
  }

  template <std::size_t... Is>
  static WritePortalType CreateWritePortalImpl(
    vtkmstd::index_sequence<Is...>,
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return WritePortalType(vtkm::tuple_element_t<Is, StorageTuple>::CreateWritePortal(
      Buffers<Is>(buffers), device, token)...);
  }

public:
  VTKM_CONT static vtkm::IdComponent GetNumberOfComponentsFlat(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    // Assume that all subcomponents are the same size. Things are not well defined otherwise.
    return vtkm::tuple_element_t<0, StorageTuple>::GetNumberOfComponentsFlat(
             GetBuffers(buffers, 0)) *
      ValueType::NUM_COMPONENTS;
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return vtkm::TupleElement<0, StorageTuple>::GetNumberOfValues(Buffers<0>(buffers));
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token)
  {
    ResizeBuffersImpl(IndexList{}, numValues, buffers, preserve, token);
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>& buffers,
                             const ValueType& fillValue,
                             vtkm::Id startIndex,
                             vtkm::Id endIndex,
                             vtkm::cont::Token& token)
  {
    FillImpl(IndexList{}, buffers, fillValue, startIndex, endIndex, token);
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return CreateReadPortalImpl(IndexList{}, buffers, device, token);
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return CreateWritePortalImpl(IndexList{}, buffers, device, token);
  }

public:
  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(
    const vtkm::cont::ArrayHandle<T, StorageTags>&... arrays)
  {
    auto numBuffers = { std::size_t{ 1 }, arrays.GetBuffers().size()... };
    Info info;
    std::partial_sum(numBuffers.begin(), numBuffers.end(), info.BufferOffset.begin());
    return vtkm::cont::internal::CreateBuffers(info, arrays...);
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers()
  {
    return CreateBuffers(vtkm::cont::ArrayHandle<T, StorageTags>{}...);
  }

private:
  using ArrayTupleType = vtkm::Tuple<vtkm::cont::ArrayHandle<T, StorageTags>...>;

  template <std::size_t... Is>
  VTKM_CONT static ArrayTupleType GetArrayTupleImpl(
    vtkmstd::index_sequence<Is...>,
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return ArrayTupleType(vtkm::cont::ArrayHandle<T, StorageTags>(Buffers<Is>(buffers))...);
  }

public:
  VTKM_CONT static ArrayTupleType GetArrayTuple(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return GetArrayTupleImpl(IndexList{}, buffers);
  }
};

// Special degenerative case when there is only one array being composited
template <typename T, typename StorageTag>
struct Storage<T, vtkm::cont::StorageTagCompositeVec<StorageTag>> : Storage<T, StorageTag>
{
  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(
    const vtkm::cont::ArrayHandle<T, StorageTag>& array = vtkm::cont::ArrayHandle<T, StorageTag>{})
  {
    return vtkm::cont::internal::CreateBuffers(array);
  }

  VTKM_CONT static vtkm::Tuple<vtkm::cont::ArrayHandle<T, StorageTag>> GetArrayTuple(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return vtkm::cont::ArrayHandle<T, StorageTag>(buffers);
  }
};

} // namespace internal

/// @brief An `ArrayHandle` that combines components from other arrays.
///
/// `ArrayHandleCompositeVector` is a specialization of `ArrayHandle` that
/// derives its content from other arrays. It takes any number of
/// single-component \c ArrayHandle objects and mimics an array that contains
/// vectors with components that come from these delegate arrays.
///
/// The easiest way to create and type an `ArrayHandleCompositeVector` is
/// to use the \c make_ArrayHandleCompositeVector functions.
///
/// The `ArrayHandleExtractComponent` class may be helpful when a desired
/// component is part of an `ArrayHandle` with a `vtkm::Vec` `ValueType`.
///
/// If you are attempted to combine components that you know are stored in
/// basic `ArrayHandle`s, consider using `ArrayHandleSOA` instead.
///
template <typename... ArrayTs>
class ArrayHandleCompositeVector
  : public ArrayHandle<typename internal::CompositeVectorTraits<ArrayTs...>::ValueType,
                       typename internal::CompositeVectorTraits<ArrayTs...>::StorageTag>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleCompositeVector,
                             (ArrayHandleCompositeVector<ArrayTs...>),
                             (typename internal::CompositeVectorTraits<ArrayTs...>::Superclass));

  /// Construct an `ArrayHandleCompositeVector` from a set of component vectors.
  VTKM_CONT
  ArrayHandleCompositeVector(const ArrayTs&... arrays)
    : Superclass(StorageType::CreateBuffers(arrays...))
  {
  }

  /// Return the arrays of all of the components in a `vtkm::Tuple` object.
  VTKM_CONT vtkm::Tuple<ArrayTs...> GetArrayTuple() const
  {
    return StorageType::GetArrayTuple(this->GetBuffers());
  }
};

/// Create a composite vector array from other arrays.
///
template <typename... ArrayTs>
VTKM_CONT ArrayHandleCompositeVector<ArrayTs...> make_ArrayHandleCompositeVector(
  const ArrayTs&... arrays)
{
  // Will issue compiler error if any of ArrayTs is not a valid ArrayHandle.
  vtkm::List<internal::compvec::VerifyArrayHandle<ArrayTs>...> checkArrayHandles;
  (void)checkArrayHandles;
  return ArrayHandleCompositeVector<ArrayTs...>(arrays...);
}

//--------------------------------------------------------------------------------
// Specialization of ArrayExtractComponent
namespace internal
{

namespace detail
{

template <typename T>
struct ExtractComponentCompositeVecFunctor
{
  using ResultArray = vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType>;

  ResultArray operator()(vtkm::IdComponent, vtkm::IdComponent, vtkm::CopyFlag) const
  {
    throw vtkm::cont::ErrorBadValue("Invalid component index given to ArrayExtractComponent.");
  }

  template <typename A0, typename... As>
  ResultArray operator()(vtkm::IdComponent compositeIndex,
                         vtkm::IdComponent subIndex,
                         vtkm::CopyFlag allowCopy,
                         const A0& array0,
                         const As&... arrays) const
  {
    if (compositeIndex == 0)
    {
      return vtkm::cont::internal::ArrayExtractComponentImpl<typename A0::StorageTag>{}(
        array0, subIndex, allowCopy);
    }
    else
    {
      return (*this)(--compositeIndex, subIndex, allowCopy, arrays...);
    }
  }
};

} // namespace detail

// Superclass will inherit the ArrayExtractComponentImplInefficient property if any
// of the sub-storage are inefficient (thus making everything inefficient).
template <typename... StorageTags>
struct ArrayExtractComponentImpl<StorageTagCompositeVec<StorageTags...>>
  : vtkm::cont::internal::ArrayExtractComponentImplInherit<StorageTags...>
{
  template <typename VecT>
  auto operator()(
    const vtkm::cont::ArrayHandle<VecT, vtkm::cont::StorageTagCompositeVec<StorageTags...>>& src,
    vtkm::IdComponent componentIndex,
    vtkm::CopyFlag allowCopy) const
  {
    using T = typename vtkm::VecTraits<VecT>::ComponentType;
    vtkm::cont::ArrayHandleCompositeVector<vtkm::cont::ArrayHandle<T, StorageTags>...> array(src);
    constexpr vtkm::IdComponent NUM_SUB_COMPONENTS = vtkm::VecFlat<T>::NUM_COMPONENTS;

    return array.GetArrayTuple().Apply(detail::ExtractComponentCompositeVecFunctor<T>{},
                                       componentIndex / NUM_SUB_COMPONENTS,
                                       componentIndex % NUM_SUB_COMPONENTS,
                                       allowCopy);
  }
};

} // namespace internal

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
    Type(obj).GetArrayTuple().ForEach(SaveFunctor{ bb });
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
