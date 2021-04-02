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
vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType>
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

  using BaseComponentType = typename vtkm::VecTraits<T>::BaseComponentType;
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

template <typename S>
struct ArrayExtractComponentImpl
{
  template <typename T>
  vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType> operator()(
    const vtkm::cont::ArrayHandle<T, S>& src,
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
  vtkm::cont::ArrayHandleStride<T> operator()(
    const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagStride>& src,
    vtkm::IdComponent componentIndex,
    vtkm::CopyFlag vtkmNotUsed(allowCopy)) const
  {
    VTKM_ASSERT(componentIndex == 0);
    return src;
  }

  template <typename T, vtkm::IdComponent N>
  auto operator()(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, vtkm::cont::StorageTagStride>& src,
                  vtkm::IdComponent componentIndex,
                  vtkm::CopyFlag allowCopy) const
    -> decltype((*this)(vtkm::cont::ArrayHandleStride<T>{}, componentIndex, allowCopy))
  {
    constexpr vtkm::IdComponent subStride = vtkm::internal::TotalNumComponents<T>::value;
    vtkm::cont::ArrayHandleStride<vtkm::Vec<T, N>> array(src);
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
vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType> ArrayExtractComponent(
  const vtkm::cont::ArrayHandle<T, S>& src,
  vtkm::IdComponent componentIndex,
  vtkm::CopyFlag allowCopy = vtkm::CopyFlag::On)
{
  return internal::ArrayExtractComponentImpl<S>{}(src, componentIndex, allowCopy);
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayExtractComponent_h
