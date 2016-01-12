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
  VTKM_EXEC_CONT_EXPORT T operator()(const T& x, const T& y) const
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
  VTKM_EXEC_CONT_EXPORT T operator()(const T& x, const T& y) const
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
  VTKM_EXEC_CONT_EXPORT T operator()(const T& x, const T& y) const
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
  VTKM_EXEC_CONT_EXPORT T operator()(const T& x, const T& y) const
  {
    return x < y ? x: y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the bitwise operation <tt>x&y</tt>
/// Note: Requires Type \p T implement the & operator.
struct BitwiseAnd
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT T operator()(const T& x, const T& y) const
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
  VTKM_EXEC_CONT_EXPORT T operator()(const T& x, const T& y) const
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
  VTKM_EXEC_CONT_EXPORT T operator()(const T& x, const T& y) const
  {
    return x ^ y;
  }
};


} // namespace vtkm

#endif //vtk_m_BinaryOperators_h
