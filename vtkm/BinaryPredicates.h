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
/// returns True if and only if \c x is not equal to \c y.
/// @note: Requires that types T and U are comparable with !=.
struct NotEqual
{
  template <typename T, typename U>
  VTKM_EXEC_CONT bool operator()(const T& x, const U& y) const
  {
    return x != y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x is equal to \c y.
/// @note: Requires that types T and U are comparable with !=.
struct Equal
{
  template <typename T, typename U>
  VTKM_EXEC_CONT bool operator()(const T& x, const U& y) const
  {
    return x == y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x is less than \c y.
/// @note: Requires that types T and U are comparable with <.
struct SortLess
{
  template <typename T, typename U>
  VTKM_EXEC_CONT bool operator()(const T& x, const U& y) const
  {
    return x < y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x is greater than \c y.
/// @note: Requires that types T and U are comparable via operator<(U, T).

struct SortGreater
{
  template <typename T, typename U>
  VTKM_EXEC_CONT bool operator()(const T& x, const U& y) const
  {
    return y < x;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x and \c y are True.
/// @note: Requires that types T and U are comparable with &&.

struct LogicalAnd
{
  template <typename T, typename U>
  VTKM_EXEC_CONT bool operator()(const T& x, const U& y) const
  {
    return x && y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x or \c y is True.
/// @note: Requires that types T and U are comparable with ||.
struct LogicalOr
{
  template <typename T, typename U>
  VTKM_EXEC_CONT bool operator()(const T& x, const U& y) const
  {
    return x || y;
  }
};

} // namespace vtkm

#endif //vtk_m_BinaryPredicates_h
