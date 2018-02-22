//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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
