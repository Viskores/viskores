//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_VectorAnalysis_h
#define vtk_m_VectorAnalysis_h

// This header file defines math functions that deal with linear albegra funcitons

#include <vtkm/Math.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{

// ----------------------------------------------------------------------------
/// \brief Returns the linear interpolation of two values based on weight
///
/// \c Lerp interpolates return the linerar interpolation of v0 and v1 based on w. v0
/// and v1 are scalars or vectors of same length. w can either be a scalar or a
/// vector of the same length as x and y. If w is outside [0,1] then lerp
/// extrapolates. If w=0 => v0 is returned if w=1 => v1 is returned.
///
template <typename ValueType, typename WeightType>
VTKM_EXEC_CONT ValueType Lerp(const ValueType& value0,
                              const ValueType& value1,
                              const WeightType& weight)
{
  return static_cast<ValueType>((WeightType(1) - weight) * value0 + weight * value1);
}
template <typename ValueType, vtkm::IdComponent N, typename WeightType>
VTKM_EXEC_CONT vtkm::Vec<ValueType, N> Lerp(const vtkm::Vec<ValueType, N>& value0,
                                            const vtkm::Vec<ValueType, N>& value1,
                                            const WeightType& weight)
{
  return (WeightType(1) - weight) * value0 + weight * value1;
}
template <typename ValueType, vtkm::IdComponent N>
VTKM_EXEC_CONT vtkm::Vec<ValueType, N> Lerp(const vtkm::Vec<ValueType, N>& value0,
                                            const vtkm::Vec<ValueType, N>& value1,
                                            const vtkm::Vec<ValueType, N>& weight)
{
  static const vtkm::Vec<ValueType, N> One(ValueType(1));
  return (One - weight) * value0 + weight * value1;
}

// ----------------------------------------------------------------------------
/// \brief Returns the square of the magnitude of a vector.
///
/// It is usually much faster to compute the square of the magnitude than the
/// square, so you should use this function in place of Magnitude or RMagnitude
/// when possible.
///
template <typename T>
VTKM_EXEC_CONT typename vtkm::VecTraits<T>::ComponentType MagnitudeSquared(const T& x)
{
  return vtkm::dot(x, x);
}

// ----------------------------------------------------------------------------
namespace detail
{
template <typename T>
VTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type MagnitudeTemplate(
  T x,
  vtkm::TypeTraitsScalarTag)
{
  return static_cast<typename detail::FloatingPointReturnType<T>::Type>(vtkm::Abs(x));
}

template <typename T>
VTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type MagnitudeTemplate(
  const T& x,
  vtkm::TypeTraitsVectorTag)
{
  return vtkm::Sqrt(vtkm::MagnitudeSquared(x));
}

} // namespace detail

/// \brief Returns the magnitude of a vector.
///
/// It is usually much faster to compute MagnitudeSquared, so that should be
/// substituted when possible (unless you are just going to take the square
/// root, which would be besides the point). On some hardware it is also faster
/// to find the reciprocal magnitude, so RMagnitude should be used if you
/// actually plan to divide by the magnitude.
///
template <typename T>
VTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type Magnitude(const T& x)
{
  return detail::MagnitudeTemplate(x, typename vtkm::TypeTraits<T>::DimensionalityTag());
}

// ----------------------------------------------------------------------------
namespace detail
{
template <typename T>
VTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type RMagnitudeTemplate(
  T x,
  vtkm::TypeTraitsScalarTag)
{
  return T(1) / vtkm::Abs(x);
}

template <typename T>
VTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type RMagnitudeTemplate(
  const T& x,
  vtkm::TypeTraitsVectorTag)
{
  return vtkm::RSqrt(vtkm::MagnitudeSquared(x));
}
} // namespace detail

/// \brief Returns the reciprocal magnitude of a vector.
///
/// On some hardware RMagnitude is faster than Magnitude, but neither is
/// as fast as MagnitudeSquared.
///
template <typename T>
VTKM_EXEC_CONT typename detail::FloatingPointReturnType<T>::Type RMagnitude(const T& x)
{
  return detail::RMagnitudeTemplate(x, typename vtkm::TypeTraits<T>::DimensionalityTag());
}

// ----------------------------------------------------------------------------
namespace detail
{
template <typename T>
VTKM_EXEC_CONT T NormalTemplate(T x, vtkm::TypeTraitsScalarTag)
{
  return vtkm::CopySign(T(1), x);
}

template <typename T>
VTKM_EXEC_CONT T NormalTemplate(const T& x, vtkm::TypeTraitsVectorTag)
{
  return vtkm::RMagnitude(x) * x;
}
} // namespace detail

/// \brief Returns a normalized version of the given vector.
///
/// The resulting vector points in the same direction but has unit length.
///
template <typename T>
VTKM_EXEC_CONT T Normal(const T& x)
{
  return detail::NormalTemplate(x, typename vtkm::TypeTraits<T>::DimensionalityTag());
}

// ----------------------------------------------------------------------------
/// \brief Changes a vector to be normal.
///
/// The given vector is scaled to be unit length.
///
template <typename T>
VTKM_EXEC_CONT void Normalize(T& x)
{
  x = vtkm::Normal(x);
}

// ----------------------------------------------------------------------------
/// \brief Find the cross product of two vectors.
///
template <typename T>
VTKM_EXEC_CONT vtkm::Vec<typename detail::FloatingPointReturnType<T>::Type, 3> Cross(
  const vtkm::Vec<T, 3>& x,
  const vtkm::Vec<T, 3>& y)
{
  return vtkm::Vec<typename detail::FloatingPointReturnType<T>::Type, 3>(
    x[1] * y[2] - x[2] * y[1], x[2] * y[0] - x[0] * y[2], x[0] * y[1] - x[1] * y[0]);
}

//-----------------------------------------------------------------------------
/// \brief Find the normal of a triangle.
///
/// Given three coordinates in space, which, unless degenerate, uniquely define
/// a triangle and the plane the triangle is on, returns a vector perpendicular
/// to that triangle/plane.
///
template <typename T>
VTKM_EXEC_CONT vtkm::Vec<typename detail::FloatingPointReturnType<T>::Type, 3>
TriangleNormal(const vtkm::Vec<T, 3>& a, const vtkm::Vec<T, 3>& b, const vtkm::Vec<T, 3>& c)
{
  return vtkm::Cross(b - a, c - a);
}

} // namespace vtkm

#endif //vtk_m_VectorAnalysis_h
