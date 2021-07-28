//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleSwizzle_h
#define vtk_m_cont_ArrayHandleSwizzle_h

#include <vtkm/cont/ArrayHandleTransform.h>

#include <vtkm/VecTraits.h>

#include <vtkmstd/integer_sequence.h>

namespace vtkm
{
namespace internal
{

template <typename InType, typename OutType>
class SwizzleFunctor
{
  using InTraits = vtkm::VecTraits<InType>;
  using InComponentType = typename InTraits::ComponentType;
  static constexpr vtkm::IdComponent NUM_IN_COMPONENTS = InTraits::NUM_COMPONENTS;

  using OutTraits = vtkm::VecTraits<OutType>;
  using OutComponentType = typename OutTraits::ComponentType;
  static constexpr vtkm::IdComponent NUM_OUT_COMPONENTS = OutTraits::NUM_COMPONENTS;

  template <vtkm::IdComponent... Is>
  using IndexSequence = vtkmstd::integer_sequence<vtkm::IdComponent, Is...>;
  using IndexList = vtkmstd::make_integer_sequence<vtkm::IdComponent, NUM_OUT_COMPONENTS>;

public:
  using MapType = vtkm::Vec<vtkm::IdComponent, NUM_OUT_COMPONENTS>;

  VTKM_CONT SwizzleFunctor(const MapType& map)
    : Map(map)
  {
  }

  VTKM_CONT SwizzleFunctor() = default;

  VTKM_EXEC_CONT OutType operator()(const InType& vec) const
  {
    return this->Swizzle(vec, IndexList{});
  }

  VTKM_CONT static MapType InitMap() { return IndexListAsMap(IndexList{}); }

private:
  template <vtkm::IdComponent... Is>
  VTKM_CONT static MapType IndexListAsMap(IndexSequence<Is...>)
  {
    return { Is... };
  }

  template <vtkm::IdComponent... Is>
  VTKM_EXEC_CONT OutType Swizzle(const InType& vec, IndexSequence<Is...>) const
  {
    return { InTraits::GetComponent(vec, this->Map[Is])... };
  }

  MapType Map = InitMap();
};

namespace detail
{

template <typename InType, typename OutType, typename Invertible>
struct GetInverseSwizzleImpl;

template <typename InType, typename OutType>
struct GetInverseSwizzleImpl<InType, OutType, std::true_type>
{
  using Type = vtkm::internal::SwizzleFunctor<OutType, InType>;
  template <typename ForwardMapType>
  VTKM_CONT static Type Value(const ForwardMapType& forwardMap)
  {
    // Note that when reversing the map, if the forwardMap repeats any indices, then
    // the map is not 1:1 and is not invertible. We cannot check that at compile time.
    // In this case, results can become unpredictible.
    using InverseMapType = typename Type::MapType;
    InverseMapType inverseMap = Type::InitMap();
    for (vtkm::IdComponent inIndex = 0; inIndex < ForwardMapType::NUM_COMPONENTS; ++inIndex)
    {
      inverseMap[forwardMap[inIndex]] = inIndex;
    }

    return Type(inverseMap);
  }
};

template <typename InType, typename OutType>
struct GetInverseSwizzleImpl<InType, OutType, std::false_type>
{
  using Type = vtkm::cont::internal::NullFunctorType;
  template <typename ForwardMapType>
  VTKM_CONT static Type Value(const ForwardMapType&)
  {
    return Type{};
  }
};

template <typename InType, typename OutType>
using SwizzleInvertible = std::integral_constant<bool,
                                                 vtkm::VecTraits<InType>::NUM_COMPONENTS ==
                                                   vtkm::VecTraits<OutType>::NUM_COMPONENTS>;

} // namespace detail

template <typename InType, typename OutType>
VTKM_CONT vtkm::internal::SwizzleFunctor<InType, OutType> GetSwizzleFunctor(
  const typename vtkm::internal::SwizzleFunctor<InType, OutType>::MapType& forwardMap)
{
  return vtkm::internal::SwizzleFunctor<InType, OutType>(forwardMap);
}

template <typename InType, typename OutType>
using InverseSwizzleType = typename detail::
  GetInverseSwizzleImpl<InType, OutType, detail::SwizzleInvertible<InType, OutType>>::Type;

template <typename InType, typename OutType>
VTKM_CONT InverseSwizzleType<InType, OutType> GetInverseSwizzleFunctor(
  const typename vtkm::internal::SwizzleFunctor<InType, OutType>::MapType& forwardMap)
{
  return detail::
    GetInverseSwizzleImpl<InType, OutType, detail::SwizzleInvertible<InType, OutType>>::Value(
      forwardMap);
}

}
} // namespace vtkm::internal

namespace vtkm
{
namespace cont
{

namespace detail
{

template <typename ArrayHandleType, vtkm::IdComponent OutSize>
struct ArrayHandleSwizzleTraits
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  using InType = typename ArrayHandleType::ValueType;
  using OutType = vtkm::Vec<typename vtkm::VecTraits<InType>::ComponentType, OutSize>;
  using SwizzleFunctor = vtkm::internal::SwizzleFunctor<InType, OutType>;
  using InverseSwizzleFunctor = vtkm::internal::InverseSwizzleType<InType, OutType>;
  using MapType = typename SwizzleFunctor::MapType;

  static SwizzleFunctor GetFunctor(const MapType& forwardMap)
  {
    return vtkm::internal::GetSwizzleFunctor<InType, OutType>(forwardMap);
  }

  static InverseSwizzleFunctor GetInverseFunctor(const MapType& forwardMap)
  {
    return vtkm::internal::GetInverseSwizzleFunctor<InType, OutType>(forwardMap);
  }

  using Superclass =
    vtkm::cont::ArrayHandleTransform<ArrayHandleType, SwizzleFunctor, InverseSwizzleFunctor>;
};

} // namespace detail

/// \brief Swizzle the components of the values in an `ArrayHandle`.
///
/// Given an `ArrayHandle` with `Vec` values, `ArrayHandleSwizzle` allows you to
/// reorder the components of all the `Vec` values. This reordering is done in place,
/// so the array does not have to be duplicated.
///
/// The resulting array does not have to contain all of the components of the input.
/// For example, you could use `ArrayHandleSwizzle` to drop one of the components of
/// each vector. However, if you do that, then the swizzled array is read-only. If
/// there is a 1:1 map from input components to output components, writing to the
/// array will be enabled.
///
/// The swizzle map given to `ArrayHandleSwizzle` must comprise valid component indices
/// (between 0 and number of components - 1). Also, the component indices should not
/// be repeated, particularly if you expect to write to the array. These conditions are
/// not checked.
///
template <typename ArrayHandleType, vtkm::IdComponent OutSize>
class ArrayHandleSwizzle
  : public detail::ArrayHandleSwizzleTraits<ArrayHandleType, OutSize>::Superclass
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  using Traits = detail::ArrayHandleSwizzleTraits<ArrayHandleType, OutSize>;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleSwizzle,
                             (ArrayHandleSwizzle<ArrayHandleType, OutSize>),
                             (typename Traits::Superclass));

  using MapType = typename Traits::MapType;

  VTKM_CONT ArrayHandleSwizzle(const ArrayHandleType& array, const MapType& map)
    : Superclass(array, Traits::GetFunctor(map), Traits::GetInverseFunctor(map))
  {
  }
};

template <typename ArrayHandleType, vtkm::IdComponent OutSize>
VTKM_CONT ArrayHandleSwizzle<ArrayHandleType, OutSize> make_ArrayHandleSwizzle(
  const ArrayHandleType& array,
  const vtkm::Vec<vtkm::IdComponent, OutSize>& map)
{
  return ArrayHandleSwizzle<ArrayHandleType, OutSize>(array, map);
}

template <typename ArrayHandleType, typename... SwizzleIndexTypes>
VTKM_CONT ArrayHandleSwizzle<ArrayHandleType, vtkm::IdComponent(sizeof...(SwizzleIndexTypes) + 1)>
make_ArrayHandleSwizzle(const ArrayHandleType& array,
                        vtkm::IdComponent swizzleIndex0,
                        SwizzleIndexTypes... swizzleIndices)
{
  return make_ArrayHandleSwizzle(array, vtkm::make_Vec(swizzleIndex0, swizzleIndices...));
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

template <typename InType, typename OutType>
struct SerializableTypeString<vtkm::internal::SwizzleFunctor<InType, OutType>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "Swizzle<" + SerializableTypeString<InType>::Get() + "," +
      SerializableTypeString<OutType>::Get() + ">";
    return name;
  }
};

template <typename AH, vtkm::IdComponent NComps>
struct SerializableTypeString<vtkm::cont::ArrayHandleSwizzle<AH, NComps>>
  : SerializableTypeString<typename vtkm::cont::ArrayHandleSwizzle<AH, NComps>::Superclass>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH, vtkm::IdComponent NComps>
struct Serialization<vtkm::cont::ArrayHandleSwizzle<AH, NComps>>
  : Serialization<typename vtkm::cont::ArrayHandleSwizzle<AH, NComps>::Superclass>
{
};

} // diy
/// @endcond SERIALIZATION

#endif // vtk_m_cont_ArrayHandleSwizzle_h
