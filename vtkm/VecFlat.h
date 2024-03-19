//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_VecFlat_h
#define vtk_m_VecFlat_h

#include <vtkm/StaticAssert.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{

namespace internal
{

template <typename T,
          typename MultipleComponents = typename vtkm::VecTraits<T>::HasMultipleComponents>
struct TotalNumComponents;

template <typename T>
struct TotalNumComponents<T, vtkm::VecTraitsTagMultipleComponents>
{
  VTKM_STATIC_ASSERT_MSG(
    (std::is_same<typename vtkm::VecTraits<T>::IsSizeStatic, vtkm::VecTraitsTagSizeStatic>::value),
    "vtkm::VecFlat can only be used with Vec types with a static number of components.");
  using ComponentType = typename vtkm::VecTraits<T>::ComponentType;
  static constexpr vtkm::IdComponent value =
    vtkm::VecTraits<T>::NUM_COMPONENTS * TotalNumComponents<ComponentType>::value;
};

template <typename T>
struct TotalNumComponents<T, vtkm::VecTraitsTagSingleComponent>
{
  static constexpr vtkm::IdComponent value = 1;
};

template <typename T>
using FlattenVec = vtkm::Vec<typename vtkm::VecTraits<T>::BaseComponentType,
                             vtkm::internal::TotalNumComponents<T>::value>;

template <typename T>
using IsFlatVec = typename std::is_same<T, FlattenVec<T>>::type;

namespace detail
{

template <typename T>
VTKM_EXEC_CONT T GetFlatVecComponentImpl(const T& component,
                                         vtkm::IdComponent index,
                                         std::true_type vtkmNotUsed(isBase))
{
  VTKM_ASSERT(index == 0);
  return component;
}

template <typename T>
VTKM_EXEC_CONT typename vtkm::VecTraits<T>::BaseComponentType
GetFlatVecComponentImpl(const T& vec, vtkm::IdComponent index, std::false_type vtkmNotUsed(isBase))
{
  using Traits = vtkm::VecTraits<T>;
  using ComponentType = typename Traits::ComponentType;
  using BaseComponentType = typename Traits::BaseComponentType;

  constexpr vtkm::IdComponent subSize = TotalNumComponents<ComponentType>::value;
  return GetFlatVecComponentImpl(Traits::GetComponent(vec, index / subSize),
                                 index % subSize,
                                 typename std::is_same<ComponentType, BaseComponentType>::type{});
}

} // namespace detail

template <typename T>
VTKM_EXEC_CONT typename vtkm::VecTraits<T>::BaseComponentType GetFlatVecComponent(
  const T& vec,
  vtkm::IdComponent index)
{
  return detail::GetFlatVecComponentImpl(vec, index, std::false_type{});
}

namespace detail
{

template <typename T, vtkm::IdComponent N>
VTKM_EXEC_CONT void CopyVecNestedToFlatImpl(T nestedVec,
                                            vtkm::Vec<T, N>& flatVec,
                                            vtkm::IdComponent flatOffset)
{
  flatVec[flatOffset] = nestedVec;
}

template <typename T, vtkm::IdComponent NFlat, vtkm::IdComponent NNest>
VTKM_EXEC_CONT void CopyVecNestedToFlatImpl(const vtkm::Vec<T, NNest>& nestedVec,
                                            vtkm::Vec<T, NFlat>& flatVec,
                                            vtkm::IdComponent flatOffset)
{
  for (vtkm::IdComponent nestedIndex = 0; nestedIndex < NNest; ++nestedIndex)
  {
    flatVec[nestedIndex + flatOffset] = nestedVec[nestedIndex];
  }
}

template <typename T, vtkm::IdComponent N, typename NestedVecType>
VTKM_EXEC_CONT void CopyVecNestedToFlatImpl(const NestedVecType& nestedVec,
                                            vtkm::Vec<T, N>& flatVec,
                                            vtkm::IdComponent flatOffset)
{
  using Traits = vtkm::VecTraits<NestedVecType>;
  using ComponentType = typename Traits::ComponentType;
  constexpr vtkm::IdComponent subSize = TotalNumComponents<ComponentType>::value;

  vtkm::IdComponent flatIndex = flatOffset;
  for (vtkm::IdComponent nestIndex = 0; nestIndex < Traits::NUM_COMPONENTS; ++nestIndex)
  {
    CopyVecNestedToFlatImpl(Traits::GetComponent(nestedVec, nestIndex), flatVec, flatIndex);
    flatIndex += subSize;
  }
}

} // namespace detail

template <typename T, vtkm::IdComponent N, typename NestedVecType>
VTKM_EXEC_CONT void CopyVecNestedToFlat(const NestedVecType& nestedVec, vtkm::Vec<T, N>& flatVec)
{
  detail::CopyVecNestedToFlatImpl(nestedVec, flatVec, 0);
}

namespace detail
{

template <typename T, vtkm::IdComponent N>
VTKM_EXEC_CONT void CopyVecFlatToNestedImpl(const vtkm::Vec<T, N>& flatVec,
                                            vtkm::IdComponent flatOffset,
                                            T& nestedVec)
{
  nestedVec = flatVec[flatOffset];
}

template <typename T, vtkm::IdComponent NFlat, vtkm::IdComponent NNest>
VTKM_EXEC_CONT void CopyVecFlatToNestedImpl(const vtkm::Vec<T, NFlat>& flatVec,
                                            vtkm::IdComponent flatOffset,
                                            vtkm::Vec<T, NNest>& nestedVec)
{
  for (vtkm::IdComponent nestedIndex = 0; nestedIndex < NNest; ++nestedIndex)
  {
    nestedVec[nestedIndex] = flatVec[nestedIndex + flatOffset];
  }
}

template <typename T, vtkm::IdComponent NFlat, typename ComponentType, vtkm::IdComponent NNest>
VTKM_EXEC_CONT void CopyVecFlatToNestedImpl(const vtkm::Vec<T, NFlat>& flatVec,
                                            vtkm::IdComponent flatOffset,
                                            vtkm::Vec<ComponentType, NNest>& nestedVec)
{
  constexpr vtkm::IdComponent subSize = TotalNumComponents<ComponentType>::value;

  vtkm::IdComponent flatIndex = flatOffset;
  for (vtkm::IdComponent nestIndex = 0; nestIndex < NNest; ++nestIndex)
  {
    CopyVecFlatToNestedImpl(flatVec, flatIndex, nestedVec[nestIndex]);
    flatIndex += subSize;
  }
}

template <typename T, vtkm::IdComponent N, typename NestedVecType>
VTKM_EXEC_CONT void CopyVecFlatToNestedImpl(const vtkm::Vec<T, N>& flatVec,
                                            vtkm::IdComponent flatOffset,
                                            NestedVecType& nestedVec)
{
  using Traits = vtkm::VecTraits<NestedVecType>;
  using ComponentType = typename Traits::ComponentType;
  constexpr vtkm::IdComponent subSize = TotalNumComponents<ComponentType>::value;

  vtkm::IdComponent flatIndex = flatOffset;
  for (vtkm::IdComponent nestIndex = 0; nestIndex < Traits::NUM_COMPONENTS; ++nestIndex)
  {
    ComponentType component;
    CopyVecFlatToNestedImpl(flatVec, flatIndex, component);
    Traits::SetComponent(nestedVec, nestIndex, component);
    flatIndex += subSize;
  }
}

} // namespace detail

template <typename T, vtkm::IdComponent N, typename NestedVecType>
VTKM_EXEC_CONT void CopyVecFlatToNested(const vtkm::Vec<T, N>& flatVec, NestedVecType& nestedVec)
{
  detail::CopyVecFlatToNestedImpl(flatVec, 0, nestedVec);
}

} // namespace internal

/// \brief Treat a `Vec` or `Vec`-like object as a flat `Vec`.
///
/// The `VecFlat` template wraps around another object that is a nested `Vec` object
/// (that is, a vector of vectors) and treats it like a flat, 1 dimensional `Vec`.
/// For example, let's say that you have a `Vec` of size 3 holding `Vec`s of size 2.
///
/// ```cpp
/// void Foo(const vtkm::Vec<vtkm::Vec<vtkm::Id, 2>, 3>& nestedVec)
/// {
///   auto flatVec = vtkm::make_VecFlat(nestedVec);
/// ```
///
/// `flatVec` is now of type `vtkm::VecFlat<vtkm::Vec<vtkm::Vec<T, 2>, 3>`.
/// `flatVec::NUM_COMPONENTS` is 6 (3 * 2). The `[]` operator takes an index between
/// 0 and 5 and returns a value of type `vtkm::Id`. The indices are explored in
/// depth-first order. So `flatVec[0] == nestedVec[0][0]`, `flatVec[1] == nestedVec[0][1]`,
/// `flatVec[2] == nestedVec[1][0]`, and so on.
///
/// Note that `flatVec` only works with types that have `VecTraits` defined where
/// the `IsSizeStatic` field is `vtkm::VecTraitsTagSizeStatic` (that is, the `NUM_COMPONENTS`
/// constant is defined).
///
template <typename T, bool = internal::IsFlatVec<T>::value>
class VecFlat;

// Case where T is not a vtkm::Vec<T, N> where T is not a Vec.
template <typename T>
class VecFlat<T, false> : public internal::FlattenVec<T>
{
  using Superclass = internal::FlattenVec<T>;

public:
  using Superclass::Superclass;
  VecFlat() = default;

  VTKM_EXEC_CONT VecFlat(const T& src) { *this = src; }

  VTKM_EXEC_CONT VecFlat& operator=(const T& src)
  {
    internal::CopyVecNestedToFlat(src, *this);
    return *this;
  }

  VTKM_EXEC_CONT operator T() const
  {
    T nestedVec;
    internal::CopyVecFlatToNested(*this, nestedVec);
    return nestedVec;
  }
};

// Specialization of VecFlat where the Vec is already flat Vec
template <typename T>
class VecFlat<T, true> : public T
{
public:
  using T::T;
  VecFlat() = default;

  VTKM_EXEC_CONT VecFlat(const T& src)
    : T(src)
  {
  }

  VTKM_EXEC_CONT VecFlat& operator=(const T& src)
  {
    this->T::operator=(src);
    return *this;
  }

  VTKM_EXEC_CONT VecFlat& operator=(T&& src)
  {
    this->T::operator=(std::move(src));
    return *this;
  }
};

/// \brief Converts a `Vec`-like object to a `VecFlat`.
///
template <typename T>
VTKM_EXEC_CONT vtkm::VecFlat<T> make_VecFlat(const T& vec)
{
  return vtkm::VecFlat<T>(vec);
}

template <typename T>
struct TypeTraits<vtkm::VecFlat<T>> : TypeTraits<internal::FlattenVec<T>>
{
};

template <typename T>
struct VecTraits<vtkm::VecFlat<T>> : VecTraits<internal::FlattenVec<T>>
{
};

} // namespace vtkm

#endif //vtk_m_VecFlat_h
