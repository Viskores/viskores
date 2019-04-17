//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_UnaryPredicates_h
#define vtk_m_UnaryPredicates_h

#include <vtkm/TypeTraits.h>
#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{

/// Predicate that takes a single argument \c x, and returns
/// True if it is the identity of the Type \p T.
struct IsZeroInitialized
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& x) const
  {
    return (x == vtkm::TypeTraits<T>::ZeroInitialization());
  }
};

/// Predicate that takes a single argument \c x, and returns
/// True if it isn't the identity of the Type \p T.
struct NotZeroInitialized
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& x) const
  {
    return (x != vtkm::TypeTraits<T>::ZeroInitialization());
  }
};

/// Predicate that takes a single argument \c x, and returns
/// True if and only if \c x is \c false.
/// Note: Requires Type \p T to be convertible to \c bool or implement the
/// ! operator.
struct LogicalNot
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& x) const
  {
    return !x;
  }
};

} // namespace vtkm

#endif //vtk_m_UnaryPredicates_h
