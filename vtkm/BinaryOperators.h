//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_BinaryOperators_h
#define vtk_m_BinaryOperators_h

#include <vtkm/Math.h>
#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{

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
/// @note Requires a suitable definition of `operator+(T, U)`.
struct Sum
{
  template <typename T, typename U>
  VTKM_EXEC_CONT auto operator()(const T& x, const U& y) const -> decltype(x + y)
  {
    return x + y;
  }

  // If both types are the same integral type, explicitly cast the result to
  // type T to avoid narrowing conversion warnings from operations that promote
  // to int (e.g. `int operator+(char, char)`)
  template <typename T>
  VTKM_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(const T& x, const T& y) const
  {
    return static_cast<T>(x + y);
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns product (multiplication) of the two values.
/// @note Requires a suitable definition of `operator*(T, U)`.
struct Product
{
  template <typename T, typename U>
  VTKM_EXEC_CONT auto operator()(const T& x, const U& y) const -> decltype(x * y)
  {
    return x * y;
  }

  // If both types are the same integral type, explicitly cast the result to
  // type T to avoid narrowing conversion warnings from operations that promote
  // to int (e.g. `int operator+(char, char)`)
  template <typename T>
  VTKM_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(const T& x, const T& y) const
  {
    return static_cast<T>(x * y);
  }
};

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the \c x if x > y otherwise returns \c y.
/// @note Requires a suitable definition of `bool operator<(T, U)` and that
/// `T` and `U` share a common type.
//needs to be full length to not clash with vtkm::math function Max.
struct Maximum
{
  template <typename T, typename U>
  VTKM_EXEC_CONT typename std::common_type<T, U>::type operator()(const T& x, const U& y) const
  {
    return x < y ? y : x;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the \c x if x < y otherwise returns \c y.
/// @note Requires a suitable definition of `bool operator<(T, U)` and that
/// `T` and `U` share a common type.
//needs to be full length to not clash with vtkm::math function Min.
struct Minimum
{
  template <typename T, typename U>
  VTKM_EXEC_CONT typename std::common_type<T, U>::type operator()(const T& x, const U& y) const
  {
    return x < y ? x : y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns a vtkm::Vec<T,2> that represents the minimum and maximum values.
/// Note: Requires Type \p T implement the vtkm::Min and vtkm::Max functions.
template <typename T>
struct MinAndMax
{
  VTKM_EXEC_CONT
  vtkm::Vec<T, 2> operator()(const T& a) const { return vtkm::make_Vec(a, a); }

  VTKM_EXEC_CONT
  vtkm::Vec<T, 2> operator()(const T& a, const T& b) const
  {
    return vtkm::make_Vec(vtkm::Min(a, b), vtkm::Max(a, b));
  }

  VTKM_EXEC_CONT
  vtkm::Vec<T, 2> operator()(const vtkm::Vec<T, 2>& a, const vtkm::Vec<T, 2>& b) const
  {
    return vtkm::make_Vec(vtkm::Min(a[0], b[0]), vtkm::Max(a[1], b[1]));
  }

  VTKM_EXEC_CONT
  vtkm::Vec<T, 2> operator()(const T& a, const vtkm::Vec<T, 2>& b) const
  {
    return vtkm::make_Vec(vtkm::Min(a, b[0]), vtkm::Max(a, b[1]));
  }

  VTKM_EXEC_CONT
  vtkm::Vec<T, 2> operator()(const vtkm::Vec<T, 2>& a, const T& b) const
  {
    return vtkm::make_Vec(vtkm::Min(a[0], b), vtkm::Max(a[1], b));
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the bitwise operation <tt>x&y</tt>
/// @note Requires a suitable definition of `operator&(T, U)`.
struct BitwiseAnd
{
  template <typename T, typename U>
  VTKM_EXEC_CONT auto operator()(const T& x, const U& y) const -> decltype(x & y)
  {
    return x & y;
  }

  // If both types are the same integral type, explicitly cast the result to
  // type T to avoid narrowing conversion warnings from operations that promote
  // to int (e.g. `int operator+(char, char)`)
  template <typename T>
  VTKM_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(const T& x, const T& y) const
  {
    return static_cast<T>(x & y);
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the bitwise operation <tt>x|y</tt>
/// @note Requires a suitable definition of `operator&(T, U)`.
struct BitwiseOr
{
  template <typename T, typename U>
  VTKM_EXEC_CONT auto operator()(const T& x, const U& y) const -> decltype(x | y)
  {
    return x | y;
  }

  // If both types are the same integral type, explicitly cast the result to
  // type T to avoid narrowing conversion warnings from operations that promote
  // to int (e.g. `int operator+(char, char)`)
  template <typename T>
  VTKM_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(const T& x, const T& y) const
  {
    return static_cast<T>(x | y);
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns the bitwise operation <tt>x^y</tt>
/// @note Requires a suitable definition of `operator&(T, U)`.
struct BitwiseXor
{
  template <typename T, typename U>
  VTKM_EXEC_CONT auto operator()(const T& x, const U& y) const -> decltype(x ^ y)
  {
    return x ^ y;
  }

  // If both types are the same integral type, explicitly cast the result to
  // type T to avoid narrowing conversion warnings from operations that promote
  // to int (e.g. `int operator+(char, char)`)
  template <typename T>
  VTKM_EXEC_CONT
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) < sizeof(int), T>::type
    operator()(const T& x, const T& y) const
  {
    return static_cast<T>(x ^ y);
  }
};

} // namespace vtkm

#endif //vtk_m_BinaryOperators_h
