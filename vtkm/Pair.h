//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_Pair_h
#define vtk_m_Pair_h

#include <vtkm/internal/Configure.h>
#include <vtkm/internal/ExportMacros.h>

#include <iostream>
#include <utility>

namespace vtkm
{

/// A \c vtkm::Pair is essentially the same as an STL pair object except that
/// the methods (constructors and operators) are defined to work in both the
/// control and execution environments (whereas std::pair is likely to work
/// only in the control environment).
///
template <typename T1, typename T2>
struct Pair
{
  /// The type of the first object.
  ///
  using FirstType = T1;

  /// The type of the second object.
  ///
  using SecondType = T2;

  /// The same as FirstType, but follows the naming convention of std::pair.
  ///
  using first_type = FirstType;

  /// The same as SecondType, but follows the naming convention of std::pair.
  ///
  using second_type = SecondType;

  /// The pair's first object. Note that this field breaks VTK-m's naming
  /// conventions to make vtkm::Pair more compatible with std::pair.
  ///
  FirstType first;

  /// The pair's second object. Note that this field breaks VTK-m's naming
  /// conventions to make vtkm::Pair more compatible with std::pair.
  ///
  SecondType second;

  Pair() = default;

  VTKM_EXEC_CONT
  Pair(const FirstType& firstSrc, const SecondType& secondSrc)
    : first(firstSrc)
    , second(secondSrc)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  Pair(FirstType&& firstSrc,
       SecondType&& secondSrc) noexcept(noexcept(FirstType{ std::declval<FirstType&&>() },
                                                 SecondType{ std::declval<SecondType&&>() }))
    : first(std::move(firstSrc))
    , second(std::move(secondSrc))
  {
  }

  Pair(const Pair&) = default;
  Pair(Pair&&) = default;

  template <typename U1, typename U2>
  VTKM_EXEC_CONT Pair(const vtkm::Pair<U1, U2>& src)
    : first(src.first)
    , second(src.second)
  {
  }

  template <typename U1, typename U2>
  VTKM_EXEC_CONT Pair(vtkm::Pair<U1, U2>&& src) noexcept(noexcept(U1{ std::declval<U1&&>() },
                                                                  U2{ std::declval<U2&&>() }))
    : first(std::move(src.first))
    , second(std::move(src.second))
  {
  }

  template <typename U1, typename U2>
  VTKM_EXEC_CONT Pair(const std::pair<U1, U2>& src)
    : first(src.first)
    , second(src.second)
  {
  }

  template <typename U1, typename U2>
  VTKM_EXEC_CONT Pair(std::pair<U1, U2>&& src) noexcept(noexcept(U1{ std::declval<U1&&>() },
                                                                 U2{ std::declval<U2&&>() }))
    : first(std::move(src.first))
    , second(std::move(src.second))
  {
  }

  vtkm::Pair<FirstType, SecondType>& operator=(const vtkm::Pair<FirstType, SecondType>& src) =
    default;
  vtkm::Pair<FirstType, SecondType>& operator=(vtkm::Pair<FirstType, SecondType>&& src) = default;

  VTKM_EXEC_CONT
  bool operator==(const vtkm::Pair<FirstType, SecondType>& other) const
  {
    return ((this->first == other.first) && (this->second == other.second));
  }

  VTKM_EXEC_CONT
  bool operator!=(const vtkm::Pair<FirstType, SecondType>& other) const
  {
    return !(*this == other);
  }

  /// Tests ordering on the first object, and then on the second object if the
  /// first are equal.
  ///
  VTKM_EXEC_CONT
  bool operator<(const vtkm::Pair<FirstType, SecondType>& other) const
  {
    return ((this->first < other.first) ||
            (!(other.first < this->first) && (this->second < other.second)));
  }

  /// Tests ordering on the first object, and then on the second object if the
  /// first are equal.
  ///
  VTKM_EXEC_CONT
  bool operator>(const vtkm::Pair<FirstType, SecondType>& other) const { return (other < *this); }

  /// Tests ordering on the first object, and then on the second object if the
  /// first are equal.
  ///
  VTKM_EXEC_CONT
  bool operator<=(const vtkm::Pair<FirstType, SecondType>& other) const { return !(other < *this); }

  /// Tests ordering on the first object, and then on the second object if the
  /// first are equal.
  ///
  VTKM_EXEC_CONT
  bool operator>=(const vtkm::Pair<FirstType, SecondType>& other) const { return !(*this < other); }
};

/// Pairwise Add.
/// This is done by adding the two objects separately.
/// Useful for Reduce operation on a zipped array
template <typename T, typename U>
VTKM_EXEC_CONT vtkm::Pair<T, U> operator+(const vtkm::Pair<T, U>& a, const vtkm::Pair<T, U>& b)
{
  return vtkm::Pair<T, U>(a.first + b.first, a.second + b.second);
}

template <typename T1, typename T2>
VTKM_EXEC_CONT vtkm::Pair<typename std::decay<T1>::type, typename std::decay<T2>::type> make_Pair(
  T1&& v1,
  T2&& v2)
{
  using DT1 = typename std::decay<T1>::type;
  using DT2 = typename std::decay<T2>::type;
  using PairT = vtkm::Pair<DT1, DT2>;

  return PairT(std::forward<T1>(v1), std::forward<T2>(v2));
}

} // namespace vtkm

#endif //vtk_m_Pair_h
