//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_Pair_h
#define vtk_m_Pair_h

#include <vtkm/internal/Configure.h>
#include <vtkm/internal/ExportMacros.h>

#include <utility>

namespace vtkm {

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
  typedef T1 FirstType;

  /// The type of the second object.
  ///
  typedef T2 SecondType;

  /// The same as FirstType, but follows the naming convention of std::pair.
  ///
  typedef FirstType first_type;

  /// The same as SecondType, but follows the naming convention of std::pair.
  ///
  typedef SecondType second_type;

  /// The pair's first object. Note that this field breaks VTK-m's naming
  /// conventions to make vtkm::Pair more compatible with std::pair.
  ///
  FirstType first;

  /// The pair's second object. Note that this field breaks VTK-m's naming
  /// conventions to make vtkm::Pair more compatible with std::pair.
  ///
  SecondType second;

  VTKM_EXEC_CONT_EXPORT
  Pair() : first(), second() {  }

  VTKM_EXEC_CONT_EXPORT
  Pair(const FirstType &firstSrc, const SecondType &secondSrc)
    : first(firstSrc), second(secondSrc) {  }

  template <typename U1, typename U2>
  VTKM_EXEC_CONT_EXPORT
  Pair(const vtkm::Pair<U1,U2> &src)
    : first(src.first), second(src.second) {  }

  template <typename U1, typename U2>
  VTKM_EXEC_CONT_EXPORT
  Pair(const std::pair<U1,U2> &src)
    : first(src.first), second(src.second) {  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Pair<FirstType,SecondType> &
  operator=(const vtkm::Pair<FirstType,SecondType> &src) {
    this->first = src.first;
    this->second = src.second;
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator==(const vtkm::Pair<FirstType,SecondType> &other) const {
    return ((this->first == other.first) && (this->second == other.second));
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator!=(const vtkm::Pair<FirstType,SecondType> &other) const {
    return !(*this == other);
  }

  /// Tests ordering on the first object, and then on the second object if the
  /// first are equal.
  ///
  VTKM_EXEC_CONT_EXPORT
  bool operator<(const vtkm::Pair<FirstType,SecondType> &other) const {
    return ((this->first < other.first)
            || (!(other.first < this->first) && (this->second < other.second)));
  }

  /// Tests ordering on the first object, and then on the second object if the
  /// first are equal.
  ///
  VTKM_EXEC_CONT_EXPORT
  bool operator>(const vtkm::Pair<FirstType,SecondType> &other) const {
    return (other < *this);
  }

  /// Tests ordering on the first object, and then on the second object if the
  /// first are equal.
  ///
  VTKM_EXEC_CONT_EXPORT
  bool operator<=(const vtkm::Pair<FirstType,SecondType> &other) const {
    return !(other < *this);
  }

  /// Tests ordering on the first object, and then on the second object if the
  /// first are equal.
  ///
  VTKM_EXEC_CONT_EXPORT
  bool operator>=(const vtkm::Pair<FirstType,SecondType> &other) const {
    return !(*this < other);
  }
};

template <typename T1, typename T2>
VTKM_EXEC_CONT_EXPORT
vtkm::Pair<T1,T2> make_Pair(const T1 &firstSrc, const T2 &secondSrc)
{
  return vtkm::Pair<T1,T2>(firstSrc, secondSrc);
}

} // namespace vtkm

#endif //vtk_m_Pair_h
