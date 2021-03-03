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

#include <vtkm/Deprecated.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/Tuple.h>
#include <vtkm/VecTraits.h>

#include <vtkm/internal/brigand.hpp>

#include <vtkmstd/integer_sequence.h>

#include <type_traits>

namespace vtkm
{
namespace internal
{

namespace compvec
{

template <typename... PortalList>
using AllPortalsAreWritable =
  typename brigand::all<brigand::list<PortalList...>,
                        brigand::bind<vtkm::internal::PortalSupportsSets, brigand::_1>>::type;

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
    return this->Portals.Apply(compvec::GetFromPortals<PortalTypes...>, index);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    this->Portals.Apply(compvec::SetToPortals<ValueType, PortalTypes...>, index, value);
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

template <std::size_t I>
struct BufferIndexImpl
{
  template <typename... Ts>
  static constexpr vtkm::IdComponent Value(vtkm::IdComponent n, Ts... remaining)
  {
    return n + BufferIndexImpl<I - 1>::Value(remaining...);
  }
};
template <>
struct BufferIndexImpl<0>
{
  template <typename... Ts>
  static constexpr vtkm::IdComponent Value(Ts...)
  {
    return 0;
  }
};

template <std::size_t I, typename... StorageTypes>
constexpr vtkm::IdComponent BufferIndex()
{
  return BufferIndexImpl<I>::Value(StorageTypes::GetNumberOfBuffers()...);
}

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
  using StorageType = Storage<ValueType, StorageTag>;
  using Superclass = ArrayHandle<ValueType, StorageTag>;
};

template <typename T, typename... StorageTags>
class Storage<vtkm::Vec<T, static_cast<vtkm::IdComponent>(sizeof...(StorageTags))>,
              vtkm::cont::StorageTagCompositeVec<StorageTags...>>
{
  template <typename S>
  using StorageFor = vtkm::cont::internal::Storage<T, S>;

  using StorageTuple = vtkm::Tuple<StorageFor<StorageTags>...>;

  template <std::size_t I>
  VTKM_CONT static constexpr vtkm::IdComponent BufferIndex()
  {
    return compvec::BufferIndex<I, StorageFor<StorageTags>...>();
  }

  template <std::size_t I, typename Buff>
  VTKM_CONT static Buff* Buffers(Buff* buffers)
  {
    return buffers + BufferIndex<I>();
  }

  using IndexList = vtkmstd::make_index_sequence<sizeof...(StorageTags)>;

public:
  using ReadPortalType =
    vtkm::internal::ArrayPortalCompositeVector<typename StorageFor<StorageTags>::ReadPortalType...>;
  using WritePortalType = vtkm::internal::ArrayPortalCompositeVector<
    typename StorageFor<StorageTags>::WritePortalType...>;

private:
  // Hoop to jump through to use Storage::ResizeBuffer in an initializer list.
  template <typename StorageType>
  static bool ResizeBuffersCallthrough(StorageType,
                                       vtkm::Id numValues,
                                       vtkm::cont::internal::Buffer* buffers,
                                       vtkm::CopyFlag preserve,
                                       vtkm::cont::Token& token)
  {
    StorageType::ResizeBuffers(numValues, buffers, preserve, token);
    return false; // Return value does not matter. Hopefully just thrown away by compiler.
  }

  template <std::size_t... Is>
  static void ResizeBuffersImpl(vtkmstd::index_sequence<Is...>,
                                vtkm::Id numValues,
                                vtkm::cont::internal::Buffer* buffers,
                                vtkm::CopyFlag preserve,
                                vtkm::cont::Token& token)
  {
    auto init_list = { ResizeBuffersCallthrough(vtkm::tuple_element_t<Is, StorageTuple>{},
                                                numValues,
                                                Buffers<Is>(buffers),
                                                preserve,
                                                token)... };
    (void)init_list;
  }

  template <std::size_t... Is>
  static ReadPortalType CreateReadPortalImpl(vtkmstd::index_sequence<Is...>,
                                             const vtkm::cont::internal::Buffer* buffers,
                                             vtkm::cont::DeviceAdapterId device,
                                             vtkm::cont::Token& token)
  {
    return ReadPortalType(vtkm::tuple_element_t<Is, StorageTuple>::CreateReadPortal(
      Buffers<Is>(buffers), device, token)...);
  }

  template <std::size_t... Is>
  static WritePortalType CreateWritePortalImpl(vtkmstd::index_sequence<Is...>,
                                               vtkm::cont::internal::Buffer* buffers,
                                               vtkm::cont::DeviceAdapterId device,
                                               vtkm::cont::Token& token)
  {
    return WritePortalType(vtkm::tuple_element_t<Is, StorageTuple>::CreateWritePortal(
      Buffers<Is>(buffers), device, token)...);
  }

public:
  VTKM_CONT constexpr static vtkm::IdComponent GetNumberOfBuffers()
  {
    return BufferIndex<sizeof...(StorageTags)>();
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(const vtkm::cont::internal::Buffer* buffers)
  {
    return vtkm::TupleElement<0, StorageTuple>::GetNumberOfValues(buffers);
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      vtkm::cont::internal::Buffer* buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token)
  {
    ResizeBuffersImpl(IndexList{}, numValues, buffers, preserve, token);
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(const vtkm::cont::internal::Buffer* buffers,
                                                   vtkm::cont::DeviceAdapterId device,
                                                   vtkm::cont::Token& token)
  {
    return CreateReadPortalImpl(IndexList{}, buffers, device, token);
  }

  VTKM_CONT static WritePortalType CreateWritePortal(vtkm::cont::internal::Buffer* buffers,
                                                     vtkm::cont::DeviceAdapterId device,
                                                     vtkm::cont::Token& token)
  {
    return CreateWritePortalImpl(IndexList{}, buffers, device, token);
  }

private:
  template <typename ArrayType>
  VTKM_CONT static bool CopyBuffers(const ArrayType& array,
                                    vtkm::cont::internal::Buffer* destBuffers)
  {
    vtkm::IdComponent numBuffers = array.GetNumberOfBuffers();
    const vtkm::cont::internal::Buffer* srcBuffers = array.GetBuffers();
    for (vtkm::IdComponent buffIndex = 0; buffIndex < numBuffers; ++buffIndex)
    {
      destBuffers[buffIndex] = srcBuffers[buffIndex];
    }
    return false; // Return value does not matter. Hopefully just thrown away by compiler.
  }

  template <std::size_t... Is, typename... ArrayTs>
  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffersImpl(
    vtkmstd::index_sequence<Is...>,
    const ArrayTs... arrays)
  {
    std::vector<vtkm::cont::internal::Buffer> buffers(
      static_cast<std::size_t>(GetNumberOfBuffers()));
    auto init_list = { CopyBuffers(arrays, Buffers<Is>(&buffers.front()))... };
    (void)init_list;
    return buffers;
  }

public:
  template <typename... ArrayTs>
  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(const ArrayTs... arrays)
  {
    return CreateBuffersImpl(IndexList{}, arrays...);
  }

private:
  using ArrayTupleType = vtkm::Tuple<vtkm::cont::ArrayHandle<T, StorageTags>...>;

  template <std::size_t... Is>
  VTKM_CONT static ArrayTupleType GetArrayTupleImpl(vtkmstd::index_sequence<Is...>,
                                                    const vtkm::cont::internal::Buffer* buffers)
  {
    return ArrayTupleType(vtkm::cont::ArrayHandle<T, StorageTags>(Buffers<Is>(buffers))...);
  }

public:
  VTKM_CONT static ArrayTupleType GetArrayTuple(const vtkm::cont::internal::Buffer* buffers)
  {
    return GetArrayTupleImpl(IndexList{}, buffers);
  }
};

// Special degenerative case when there is only one array being composited
template <typename T, typename StorageTag>
struct Storage<T, vtkm::cont::StorageTagCompositeVec<StorageTag>> : Storage<T, StorageTag>
{
  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(
    const vtkm::cont::ArrayHandle<T, StorageTag>& array)
  {
    return vtkm::cont::internal::CreateBuffers(array);
  }

  VTKM_CONT static vtkm::Tuple<vtkm::cont::ArrayHandle<T, StorageTag>> GetArrayTuple(
    const vtkm::cont::internal::Buffer* buffers)
  {
    return vtkm::cont::ArrayHandle<T, StorageTag>(buffers);
  }
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
  using StorageType = typename Traits::StorageType;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleCompositeVector,
                             (ArrayHandleCompositeVector<ArrayTs...>),
                             (typename Traits::Superclass));

  VTKM_CONT
  ArrayHandleCompositeVector(const ArrayTs&... arrays)
    : Superclass(StorageType::CreateBuffers(arrays...))
  {
  }

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

template <typename... StorageTags>
struct ArrayExtractComponentImpl<StorageTagCompositeVec<StorageTags...>>
{
  template <typename T, vtkm::IdComponent NUM_COMPONENTS>
  typename detail::ExtractComponentCompositeVecFunctor<T>::ResultArray operator()(
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, NUM_COMPONENTS>,
                                  vtkm::cont::StorageTagCompositeVec<StorageTags...>>& src,
    vtkm::IdComponent componentIndex,
    vtkm::CopyFlag allowCopy) const
  {
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
