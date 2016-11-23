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
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_BinaryOperators_h
#define vtk_m_BinaryOperators_h

#include <vtkm/internal/ExportMacros.h>
#include <vtkm/Math.h>
#include <vtkm/Pair.h>

namespace vtkm {

// Disable conversion warnings for Sum and Product on GCC only.
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns sum (addition) of the two values.
/// Note: Requires Type \p T implement the + operator.
struct Sum
{
  template<typename T>
  VTKM_EXEC_CONT T operator()(const T& x, const T& y) const
  {
    return x + y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns product (multiplication) of the two values.
/// Note: Requires Type \p T implement the * operator.
struct Product
{
  template<typename T>
  VTKM_EXEC_CONT T operator()(const T& x, const T& y) const
  {
    return x * y;
  }
};

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang


/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the \c x if x > y otherwise returns \c y.
/// Note: Requires Type \p T implement the < operator.
//needs to be full length to not clash with vtkm::math function Max.
struct Maximum
{
  template<typename T>
  VTKM_EXEC_CONT T operator()(const T& x, const T& y) const
  {
    return x < y ? y: x;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the \c x if x < y otherwise returns \c y.
/// Note: Requires Type \p T implement the < operator.
//needs to be full length to not clash with vtkm::math function Min.
struct Minimum
{
  template<typename T>
  VTKM_EXEC_CONT T operator()(const T& x, const T& y) const
  {
    return x < y ? x: y;
  }
};


/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns a vtkm::Pair<T,T> that represents the minimum and maximum values
/// Note: Requires Type \p T implement the vtkm::Min and vtkm::Max functions.
template <typename T>
struct MinAndMax
{
  VTKM_EXEC_CONT
  vtkm::Pair<T, T> operator()(const T& a, const T& b) const
  {
    return vtkm::make_Pair(vtkm::Min(a, b), vtkm::Max(a, b));
  }

  VTKM_EXEC_CONT
  vtkm::Pair<T, T> operator()(
    const vtkm::Pair<T, T>& a, const vtkm::Pair<T, T>& b) const
  {
    return vtkm::make_Pair(
      vtkm::Min(a.first, b.first), vtkm::Max(a.second, b.second));
  }

  VTKM_EXEC_CONT
  vtkm::Pair<T, T> operator()(const T& a, const vtkm::Pair<T, T>& b) const
  {
    return vtkm::make_Pair(vtkm::Min(a, b.first), vtkm::Max(a, b.second));
  }

  VTKM_EXEC_CONT
  vtkm::Pair<T, T> operator()(const vtkm::Pair<T, T>& a, const T& b) const
  {
    return vtkm::make_Pair(vtkm::Min(a.first, b), vtkm::Max(a.second, b));
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the bitwise operation <tt>x&y</tt>
/// Note: Requires Type \p T implement the & operator.
struct BitwiseAnd
{
  template<typename T>
  VTKM_EXEC_CONT T operator()(const T& x, const T& y) const
  {
    return x & y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the bitwise operation <tt>x|y</tt>
/// Note: Requires Type \p T implement the | operator.
struct BitwiseOr
{
  template<typename T>
  VTKM_EXEC_CONT T operator()(const T& x, const T& y) const
  {
    return x | y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the bitwise operation <tt>x^y</tt>
/// Note: Requires Type \p T implement the ^ operator.
struct BitwiseXor
{
  template<typename T>
  VTKM_EXEC_CONT T operator()(const T& x, const T& y) const
  {
    return x ^ y;
  }
};


} // namespace vtkm

#endif //vtk_m_BinaryOperators_h
