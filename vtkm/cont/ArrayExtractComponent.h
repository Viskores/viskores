//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayExtractComponent_h
#define vtk_m_cont_ArrayExtractComponent_h

#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/ArrayHandleStride.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Logging.h>

#include <vtkm/TypeTraits.h>
#include <vtkm/VecFlat.h>
#include <vtkm/VecTraits.h>

#include <vtkmstd/integer_sequence.h>

#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

// Note: Using partial template specialization instead of function overloading to
// specialize ArrayExtractComponent for different types of array handles. This is
// because function overloading from a templated function is done when the template
// is defined rather than where it is resolved. This causes problems when extracting
// components of, say, an ArrayHandleMultiplexer holding an ArrayHandleSOA.
template <typename T, typename S>
vtkm::cont::ArrayHandleStride<typename vtkm::internal::SafeVecTraits<T>::BaseComponentType>
ArrayExtractComponentFallback(const vtkm::cont::ArrayHandle<T, S>& src,
                              vtkm::IdComponent componentIndex,
                              vtkm::CopyFlag allowCopy)
{
  if (allowCopy != vtkm::CopyFlag::On)
  {
    throw vtkm::cont::ErrorBadValue("Cannot extract component of " +
                                    vtkm::cont::TypeToString<vtkm::cont::ArrayHandle<T, S>>() +
                                    " without copying");
  }
  VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
             "Extracting component " << componentIndex << " of "
                                     << vtkm::cont::TypeToString<vtkm::cont::ArrayHandle<T, S>>()
                                     << " requires an inefficient memory copy.");

  using BaseComponentType = typename vtkm::internal::SafeVecTraits<T>::BaseComponentType;
  vtkm::Id numValues = src.GetNumberOfValues();
  vtkm::cont::ArrayHandleBasic<BaseComponentType> dest;
  dest.Allocate(numValues);
  auto srcPortal = src.ReadPortal();
  auto destPortal = dest.WritePortal();
  for (vtkm::Id arrayIndex = 0; arrayIndex < numValues; ++arrayIndex)
  {
    destPortal.Set(arrayIndex,
                   vtkm::internal::GetFlatVecComponent(srcPortal.Get(arrayIndex), componentIndex));
  }

  return vtkm::cont::ArrayHandleStride<BaseComponentType>(dest, numValues, 1, 0);
}

// Used as a superclass for ArrayHandleComponentImpls that are inefficient (and should be
// avoided).
struct ArrayExtractComponentImplInefficient
{
};

template <typename S>
struct ArrayExtractComponentImpl : ArrayExtractComponentImplInefficient
{
  template <typename T>
  vtkm::cont::ArrayHandleStride<typename vtkm::internal::SafeVecTraits<T>::BaseComponentType>
  operator()(const vtkm::cont::ArrayHandle<T, S>& src,
             vtkm::IdComponent componentIndex,
             vtkm::CopyFlag allowCopy) const
  {
    // This is the slow "default" implementation. ArrayHandle implementations should provide
    // more efficient overloads where applicable.
    return vtkm::cont::internal::ArrayExtractComponentFallback(src, componentIndex, allowCopy);
  }
};

template <>
struct ArrayExtractComponentImpl<vtkm::cont::StorageTagStride>
{
  template <typename T>
  vtkm::cont::ArrayHandleStride<typename vtkm::internal::SafeVecTraits<T>::BaseComponentType>
  operator()(const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagStride>& src,
             vtkm::IdComponent componentIndex,
             vtkm::CopyFlag allowCopy) const
  {
    return this->DoExtract(src,
                           componentIndex,
                           allowCopy,
                           typename vtkm::internal::SafeVecTraits<T>::HasMultipleComponents{});
  }

private:
  template <typename T>
  auto DoExtract(const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagStride>& src,
                 vtkm::IdComponent componentIndex,
                 vtkm::CopyFlag vtkmNotUsed(allowCopy),
                 vtkm::VecTraitsTagSingleComponent) const
  {
    VTKM_ASSERT(componentIndex == 0);
    using VTraits = vtkm::internal::SafeVecTraits<T>;
    using TBase = typename VTraits::BaseComponentType;
    VTKM_STATIC_ASSERT(VTraits::NUM_COMPONENTS == 1);

    vtkm::cont::ArrayHandleStride<T> array(src);

    // Note, we are initializing the result in this strange way for cases where type
    // T has a single component but does not equal its own BaseComponentType. A vtkm::Vec
    // of size 1 fits into this category.
    return vtkm::cont::ArrayHandleStride<TBase>(array.GetBuffers()[1],
                                                array.GetNumberOfValues(),
                                                array.GetStride(),
                                                array.GetOffset(),
                                                array.GetModulo(),
                                                array.GetDivisor());
  }

  template <typename VecType>
  auto DoExtract(const vtkm::cont::ArrayHandle<VecType, vtkm::cont::StorageTagStride>& src,
                 vtkm::IdComponent componentIndex,
                 vtkm::CopyFlag allowCopy,
                 vtkm::VecTraitsTagMultipleComponents) const
  {
    using VTraits = vtkm::internal::SafeVecTraits<VecType>;
    using T = typename VTraits::ComponentType;
    constexpr vtkm::IdComponent N = VTraits::NUM_COMPONENTS;

    constexpr vtkm::IdComponent subStride = vtkm::internal::TotalNumComponents<T>::value;
    vtkm::cont::ArrayHandleStride<VecType> array(src);
    vtkm::cont::ArrayHandleStride<T> tmpIn(array.GetBuffers()[1],
                                           array.GetNumberOfValues(),
                                           array.GetStride() * N,
                                           (array.GetOffset() * N) + (componentIndex / subStride),
                                           array.GetModulo() * N,
                                           array.GetDivisor());
    return (*this)(tmpIn, componentIndex % subStride, allowCopy);
  }
};

template <>
struct ArrayExtractComponentImpl<vtkm::cont::StorageTagBasic>
{
  template <typename T>
  auto operator()(const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>& src,
                  vtkm::IdComponent componentIndex,
                  vtkm::CopyFlag allowCopy) const
    -> decltype(
      ArrayExtractComponentImpl<vtkm::cont::StorageTagStride>{}(vtkm::cont::ArrayHandleStride<T>{},
                                                                componentIndex,
                                                                allowCopy))
  {
    return ArrayExtractComponentImpl<vtkm::cont::StorageTagStride>{}(
      vtkm::cont::ArrayHandleStride<T>(src, src.GetNumberOfValues(), 1, 0),
      componentIndex,
      allowCopy);
  }
};

namespace detail
{

template <std::size_t, typename Super>
struct ForwardSuper : Super
{
};

template <typename sequence, typename... Supers>
struct SharedSupersImpl;

template <std::size_t... Indices, typename... Supers>
struct SharedSupersImpl<vtkmstd::index_sequence<Indices...>, Supers...>
  : ForwardSuper<Indices, Supers>...
{
};

} // namespace detail

// `ArrayExtractComponentImpl`s that modify the behavior from other storage types might
// want to inherit from the `ArrayExtractComponentImpl`s of these storage types. However,
// if the template specifies multiple storage types, two of the same might be specified,
// and it is illegal in C++ to directly inherit from the same type twice. This special
// superclass accepts a variable amout of superclasses. Inheriting from this will inherit
// from all these superclasses, and duplicates are allowed.
template <typename... Supers>
using DuplicatedSuperclasses =
  detail::SharedSupersImpl<vtkmstd::make_index_sequence<sizeof...(Supers)>, Supers...>;

template <typename... StorageTags>
using ArrayExtractComponentImplInherit =
  DuplicatedSuperclasses<vtkm::cont::internal::ArrayExtractComponentImpl<StorageTags>...>;

/// \brief Resolves to true if ArrayHandleComponent of the array handle would be inefficient.
///
template <typename ArrayHandleType>
using ArrayExtractComponentIsInefficient = typename std::is_base_of<
  vtkm::cont::internal::ArrayExtractComponentImplInefficient,
  vtkm::cont::internal::ArrayExtractComponentImpl<typename ArrayHandleType::StorageTag>>::type;

} // namespace internal

/// \brief Pulls a component out of an `ArrayHandle`.
///
/// Given an `ArrayHandle` of any type, `ArrayExtractComponent` returns an
/// `ArrayHandleStride` of the base component type that contains the data for the
/// specified array component. This function can be used to apply an operation on
/// an `ArrayHandle` one component at a time. Because the array type is always
/// `ArrayHandleStride`, you can drastically cut down on the number of templates
/// to instantiate (at a possible cost to performance).
///
/// Note that `ArrayExtractComponent` will flatten out the indices of any vec value
/// type and return an `ArrayExtractComponent` of the base component type. For
/// example, if you call `ArrayExtractComponent` on an `ArrayHandle` with a value
/// type of `vtkm::Vec<vtkm::Vec<vtkm::Float32, 2>, 3>`, you will get an
/// `ArrayExtractComponent<vtkm::Float32>` returned. The `componentIndex` provided
/// will be applied to the nested vector in depth first order. So in the previous
/// example, a `componentIndex` of 0 gets the values at [0][0], `componentIndex`
/// of 1 gets [0][1], `componentIndex` of 2 gets [1][0], and so on.
///
/// Some `ArrayHandle`s allow this method to return an `ArrayHandleStride` that
/// shares the same memory as the the original `ArrayHandle`. This form will be
/// used if possible. In this case, if data are written into the `ArrayHandleStride`,
/// they are also written into the original `ArrayHandle`. However, other forms will
/// require copies into a new array. In this case, writes into `ArrayHandleStride`
/// will not affect the original `ArrayHandle`.
///
/// For some operations, such as writing into an output array, this behavior of
/// shared arrays is necessary. For this case, the optional argument `allowCopy`
/// can be set to `vtkm::CopyFlag::Off` to prevent the copying behavior into the
/// return `ArrayHandleStride`. If this is the case, an `ErrorBadValue` is thrown.
/// If the arrays can be shared, they always will be regardless of the value of
/// `allowCopy`.
///
/// Many forms of `ArrayHandle` have optimized versions to pull out a component.
/// Some, however, do not. In these cases, a fallback array copy, done in serial,
/// will be performed. A warning will be logged to alert users of this likely
/// performance bottleneck.
///
/// As an implementation note, this function should not be overloaded directly.
/// Instead, `ArrayHandle` implementations should provide a specialization of
/// `vtkm::cont::internal::ArrayExtractComponentImpl`.
///
template <typename T, typename S>
vtkm::cont::ArrayHandleStride<typename vtkm::internal::SafeVecTraits<T>::BaseComponentType>
ArrayExtractComponent(const vtkm::cont::ArrayHandle<T, S>& src,
                      vtkm::IdComponent componentIndex,
                      vtkm::CopyFlag allowCopy = vtkm::CopyFlag::On)
{
  return internal::ArrayExtractComponentImpl<S>{}(src, componentIndex, allowCopy);
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayExtractComponent_h
