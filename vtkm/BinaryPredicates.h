//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_BinaryPredicates_h
#define vtk_m_BinaryPredicates_h

#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x is equal to \c y.
/// Note: Requires Type \p T implement the == operator.
struct Equal
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& x, const T& y) const
  {
    return x == y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x is not equal to \c y.
/// Note: Requires Type \p T implement the != operator.
struct NotEqual
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& x, const T& y) const
  {
    return x != y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x is less than \c y.
/// Note: Requires Type \p T implement the < operator.
struct SortLess
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& x, const T& y) const
  {
    return x < y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x is greater than \c y.
/// Note: Requires Type \p T implement the < operator, as we invert the
/// comparison
struct SortGreater
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& x, const T& y) const
  {
    return y < x;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x and \c y are True.
/// Note: Requires Type \p T to be convertible to \c bool or implement the
/// && operator.
struct LogicalAnd
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& x, const T& y) const
  {
    return x && y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x or \c y is True.
/// Note: Requires Type \p T to be convertible to \c bool or implement the
/// || operator.
struct LogicalOr
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& x, const T& y) const
  {
    return x || y;
  }
};

} // namespace vtkm

#endif //vtk_m_BinaryPredicates_h
