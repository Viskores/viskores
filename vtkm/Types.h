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
#ifndef vtk_m_Types_h
#define vtk_m_Types_h


#include <vtkm/internal/Configure.h>
#include <vtkm/internal/ExportMacros.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/utility/enable_if.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

/*!
 * \namespace vtkm
 * \brief VTKm Toolkit.
 *
 * vtkm is the namespace for the VTKm Toolkit. It contains other sub namespaces,
 * as well as basic data types and functions callable from all components in VTKm
 * toolkit.
 *
 * \namespace vtkm::cont
 * \brief VTKm Control Environment.
 *
 * vtkm::cont defines the publicly accessible API for the VTKm Control
 * Environment. Users of the VTKm Toolkit can use this namespace to access the
 * Control Environment.
 *
 * \namespace vtkm::cuda
 * \brief CUDA implementation.
 *
 * vtkm::cuda includes the code to implement the VTKm for CUDA-based platforms.
 *
 * \namespace vtkm::cuda::cont
 * \brief CUDA implementation for Control Environment.
 *
 * vtkm::cuda::cont includes the code to implement the VTKm Control Environment
 * for CUDA-based platforms.
 *
 * \namespace vtkm::cuda::exec
 * \brief CUDA implementation for Execution Environment.
 *
 * vtkm::cuda::exec includes the code to implement the VTKm Execution Environment
 * for CUDA-based platforms.
 *
 * \namespace vtkm::exec
 * \brief VTKm Execution Environment.
 *
 * vtkm::exec defines the publicly accessible API for the VTKm Execution
 * Environment. Worklets typically use classes/apis defined within this
 * namespace alone.
 *
 * \namespace vtkm::internal
 * \brief VTKm Internal Environment
 *
 * vtkm::internal defines API which is internal and subject to frequent
 * change. This should not be used for projects using VTKm. Instead it servers
 * are a reference for the developers of VTKm.
 *
 * \namespace vtkm::math
 * \brief Utility math functions
 *
 * vtkm::math defines the publicly accessible API for Utility Math functions.
 *
 * \namespace vtkm::testing
 * \brief Internal testing classes
 *
 */

namespace vtkm {
//*****************************************************************************
// Typedefs for basic types.
//*****************************************************************************

/// Alignment requirements are prescribed by CUDA on device (Table B-1 in NVIDIA
/// CUDA C Programming Guide 4.0)

#if VTKM_SIZE_FLOAT == 4
typedef float Float32;
#else
#error Could not find a 32-bit float.
#endif

#if VTKM_SIZE_DOUBLE == 8
typedef double Float64;
#else
#error Could not find a 64-bit float.
#endif

#if VTKM_SIZE_CHAR == 1
typedef signed char Int8;
typedef unsigned char UInt8;
#else
#error Could not find an 8-bit integer.
#endif

#if VTKM_SIZE_SHORT == 2
typedef signed short Int16;
typedef unsigned short UInt16;
#else
#error Could not find a 16-bit integer.
#endif

#if VTKM_SIZE_INT == 4
typedef signed int Int32;
typedef unsigned int UInt32;
#else
#error Could not find a 32-bit integer.
#endif

#if VTKM_SIZE_LONG == 8
typedef signed long Int64;
typedef unsigned long UInt64;
#elif VTKM_SIZE_LONG_LONG == 8
typedef signed long long Int64;
typedef unsigned long long UInt64;
#else
#error Could not find a 64-bit integer.
#endif

//-----------------------------------------------------------------------------

#if VTKM_SIZE_ID == 4

/// Represents an ID (index into arrays).
typedef vtkm::Int32 Id;

#elif VTKM_SIZE_ID == 8

/// Represents an ID.
typedef vtkm::Int64 Id;

#else
#error Unknown Id Size
#endif

/// Represents a component ID (index of component in a vector). The number
/// of components, being a value fixed at compile time, is generally assumed
/// to be quite small. However, we are currently using a 32-bit width
/// integer because modern processors tend to access them more efficiently
/// than smaller widths.
typedef vtkm::Int32 IdComponent;

#ifdef VTKM_USE_DOUBLE_PRECISION

/// The floating point type to use when no other precision is specified.
typedef vtkm::Float64 FloatDefault;

#else //VTKM_USE_DOUBLE_PRECISION

/// The floating point type to use when no other precision is specified.
typedef vtkm::Float32 FloatDefault;

#endif //VTKM_USE_DOUBLE_PRECISION

namespace internal {

//-----------------------------------------------------------------------------

template<vtkm::IdComponent Size>
struct VecEquals
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a, const T& b) const
  {
    bool equal = true;
    for (vtkm::IdComponent componentIndex = 0;
         equal && (componentIndex < Size);
         componentIndex++)
    {
      equal &= a[componentIndex] == b[componentIndex];
    }
    return equal;
  }
};

template<>
struct VecEquals<1>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a, const T& b) const
  {
    return a[0] == b[0];
  }
};

template<>
struct VecEquals<2>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a, const T& b) const
  {
    return ((a[0] == b[0]) && (a[1] == b[1]));
  }
};

template<>
struct VecEquals<3>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a, const T& b) const
  {
    return ((a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]));
  }
};

template<>
struct VecEquals<4>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a, const T& b) const
  {
    return ((a[0] == b[0])
            && (a[1] == b[1])
            && (a[2] == b[2])
            && (a[3] == b[3]));
  }
};

template<vtkm::IdComponent Size>
struct AssignScalarToVec
{
  template<typename VectorType, typename ComponentType>
  VTKM_EXEC_CONT_EXPORT
  void operator()(VectorType &dest, const ComponentType &src)
  {
    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < Size;
         componentIndex++)
    {
      dest[componentIndex] = src;
    }
  }
};

template<>
struct AssignScalarToVec<1>
{
  template<typename VectorType, typename ComponentType>
  VTKM_EXEC_CONT_EXPORT
  void operator()(VectorType &dest, const ComponentType &src)
  {
    dest[0] = src;
  }
};

template<>
struct AssignScalarToVec<2>
{
  template<typename VectorType, typename ComponentType>
  VTKM_EXEC_CONT_EXPORT
  void operator()(VectorType &dest, const ComponentType &src)
  {
    dest[0] = src;
    dest[1] = src;
  }
};

template<>
struct AssignScalarToVec<3>
{
  template<typename VectorType, typename ComponentType>
  VTKM_EXEC_CONT_EXPORT
  void operator()(VectorType &dest, const ComponentType &src)
  {
    dest[0] = src;
    dest[1] = src;
    dest[2] = src;
  }
};

template<>
struct AssignScalarToVec<4>
{
  template<typename VectorType, typename ComponentType>
  VTKM_EXEC_CONT_EXPORT
  void operator()(VectorType &dest, const ComponentType &src)
  {
    dest[0] = src;
    dest[1] = src;
    dest[2] = src;
    dest[3] = src;
  }
};

template<typename CType, vtkm::IdComponent Size>
struct VecCopy
{
  template<typename T1, typename T2>
  VTKM_EXEC_CONT_EXPORT void operator()(T1 &dest, const T2 &src)
  {
    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < Size;
         componentIndex++)
    {
      dest[componentIndex] = CType(src[componentIndex]);
    }
  }
};

template<typename CType>
struct VecCopy<CType, 1>
{
  template<typename T1, typename T2>
  VTKM_EXEC_CONT_EXPORT void operator()(T1 &dest, const T2 &src)
  {
    dest[0] = CType(src[0]);
  }
};

template<typename CType>
struct VecCopy<CType, 2>
{
  template<typename T1, typename T2>
  VTKM_EXEC_CONT_EXPORT void operator()(T1 &dest, const T2 &src)
  {
    dest[0] = CType(src[0]);
    dest[1] = CType(src[1]);
  }
};

template<typename CType>
struct VecCopy<CType, 3>
{
  template<typename T1, typename T2>
  VTKM_EXEC_CONT_EXPORT void operator()(T1 &dest, const T2 &src)
  {
    dest[0] = CType(src[0]);
    dest[1] = CType(src[1]);
    dest[2] = CType(src[2]);
  }
};

template<typename CType>
struct VecCopy<CType, 4>
{
  template<typename T1, typename T2>
  VTKM_EXEC_CONT_EXPORT void operator()(T1 &dest, const T2 &src)
  {
    dest[0] = CType(src[0]);
    dest[1] = CType(src[1]);
    dest[2] = CType(src[2]);
    dest[3] = CType(src[3]);
  }
};

template<vtkm::IdComponent Size>
struct VecSum
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  typename T::ComponentType operator()(const T &x)
  {
    typename T::ComponentType sum = x[0];
    for (vtkm::IdComponent componentIndex = 1;
         componentIndex < Size;
         componentIndex++)
    {
      sum += x[componentIndex];
    }
    return sum;
  }
};

template<>
struct VecSum<0>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  typename T::ComponentType operator()(const T &)
  {
    return T::ComponentType(0);
  }
};

template<>
struct VecSum<1>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  typename T::ComponentType operator()(const T &x)
  {
    return x[0];
  }
};

template<>
struct VecSum<2>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  typename T::ComponentType operator()(const T &x)
  {
    return x[0] + x[1];
  }
};

template<>
struct VecSum<3>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  typename T::ComponentType operator()(const T &x)
  {
    return x[0] + x[1] + x[2];
  }
};

template<>
struct VecSum<4>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  typename T::ComponentType operator()(const T &x)
  {
    return x[0] + x[1] + x[2] + x[3];
  }
};

template<vtkm::IdComponent Size>
struct VecProduct
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  typename T::ComponentType operator()(const T &x)
  {
    typename T::ComponentType product = x[0];
    for (vtkm::IdComponent componentIndex = 1;
         componentIndex < Size;
         componentIndex++)
    {
      product *= x[componentIndex];
    }
    return product;
  }
};

template<>
struct VecProduct<0>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  typename T::ComponentType operator()(const T &)
  {
    return T::ComponentType(1);
  }
};

template<>
struct VecProduct<1>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  typename T::ComponentType operator()(const T &x)
  {
    return x[0];
  }
};

template<>
struct VecProduct<2>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  typename T::ComponentType operator()(const T &x)
  {
    return x[0] * x[1];
  }
};

template<>
struct VecProduct<3>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  typename T::ComponentType operator()(const T &x)
  {
    return x[0] * x[1] * x[2];
  }
};

template<>
struct VecProduct<4>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT
  typename T::ComponentType operator()(const T &x)
  {
    return x[0] * x[1] * x[2] * x[3];
  }
};

template<vtkm::IdComponent Size>
struct VecComponentWiseBinaryOperation
{
  template<typename T, typename BinaryOpType>
  VTKM_EXEC_CONT_EXPORT
  T operator()(const T &a, const T &b, const BinaryOpType &binaryOp) const
  {
    T result;
    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < Size;
         componentIndex++)
    {
      result[componentIndex] = binaryOp(a[componentIndex], b[componentIndex]);
    }
    return result;
  }
};

template<>
struct VecComponentWiseBinaryOperation<1>
{
  template<typename T, typename BinaryOpType>
  VTKM_EXEC_CONT_EXPORT
  T operator()(const T &a, const T &b, const BinaryOpType &binaryOp) const
  {
    return T(binaryOp(a[0], b[0]));
  }
};

template<>
struct VecComponentWiseBinaryOperation<2>
{
  template<typename T, typename BinaryOpType>
  VTKM_EXEC_CONT_EXPORT
  T operator()(const T &a, const T &b, const BinaryOpType &binaryOp) const
  {
    return T(binaryOp(a[0], b[0]),
             binaryOp(a[1], b[1]));
  }
};

template<>
struct VecComponentWiseBinaryOperation<3>
{
  template<typename T, typename BinaryOpType>
  VTKM_EXEC_CONT_EXPORT
  T operator()(const T &a, const T &b, const BinaryOpType &binaryOp) const
  {
    return T(binaryOp(a[0], b[0]),
             binaryOp(a[1], b[1]),
             binaryOp(a[2], b[2]));
  }
};

template<>
struct VecComponentWiseBinaryOperation<4>
{
  template<typename T, typename BinaryOpType>
  VTKM_EXEC_CONT_EXPORT
  T operator()(const T &a, const T &b, const BinaryOpType &binaryOp) const
  {
    return T(binaryOp(a[0], b[0]),
             binaryOp(a[1], b[1]),
             binaryOp(a[2], b[2]),
             binaryOp(a[3], b[3]));
  }
};

template<vtkm::IdComponent Size>
struct VecComponentWiseUnaryOperation
{
  template<typename T, typename UnaryOpType>
  VTKM_EXEC_CONT_EXPORT
  T operator()(const T &v, const UnaryOpType &unaryOp) const
  {
    T result;
    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < Size;
         componentIndex++)
    {
      result[componentIndex] = unaryOp(v[componentIndex]);
    }
    return result;
  }
};

template<>
struct VecComponentWiseUnaryOperation<1>
{
  template<typename T, typename UnaryOpType>
  VTKM_EXEC_CONT_EXPORT
  T operator()(const T &v, const UnaryOpType &unaryOp) const
  {
    return T(unaryOp(v[0]));
  }
};

template<>
struct VecComponentWiseUnaryOperation<2>
{
  template<typename T, typename UnaryOpType>
  VTKM_EXEC_CONT_EXPORT
  T operator()(const T &v, const UnaryOpType &unaryOp) const
  {
    return T(unaryOp(v[0]), unaryOp(v[1]));
  }
};

template<>
struct VecComponentWiseUnaryOperation<3>
{
  template<typename T, typename UnaryOpType>
  VTKM_EXEC_CONT_EXPORT
  T operator()(const T &v, const UnaryOpType &unaryOp) const
  {
    return T(unaryOp(v[0]), unaryOp(v[1]), unaryOp(v[2]));
  }
};

template<>
struct VecComponentWiseUnaryOperation<4>
{
  template<typename T, typename UnaryOpType>
  VTKM_EXEC_CONT_EXPORT
  T operator()(const T &v, const UnaryOpType &unaryOp) const
  {
    return T(unaryOp(v[0]), unaryOp(v[1]), unaryOp(v[2]), unaryOp(v[3]));
  }
};

template<typename T, typename BinaryOpType>
struct BindLeftBinaryOp
{
  // Warning: a reference.
  const T &LeftValue;
  const BinaryOpType BinaryOp;
  VTKM_EXEC_CONT_EXPORT
  BindLeftBinaryOp(const T &leftValue, BinaryOpType binaryOp = BinaryOpType())
    : LeftValue(leftValue), BinaryOp(binaryOp) {  }
  VTKM_EXEC_CONT_EXPORT
  T operator()(const T &rightValue) const
  {
    return this->BinaryOp(this->LeftValue, rightValue);
  }
};

template<typename T, typename BinaryOpType>
struct BindRightBinaryOp
{
  // Warning: a reference.
  const T &RightValue;
  const BinaryOpType BinaryOp;
  VTKM_EXEC_CONT_EXPORT
  BindRightBinaryOp(const T &rightValue, BinaryOpType binaryOp = BinaryOpType())
    : RightValue(rightValue), BinaryOp(binaryOp) {  }
  VTKM_EXEC_CONT_EXPORT
  T operator()(const T &leftValue) const
  {
    return this->BinaryOp(leftValue, this->RightValue);
  }
};

// Disable conversion warnings for Add, Subtract, Multiply, Divide on GCC only.
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
#if defined(VTKM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang
struct Add
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT T operator()(const T &a, const T &b) const
  {
    return T(a + b);
  }
};

struct Subtract
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT T operator()(const T &a, const T &b) const
  {
    return T(a - b);
  }
};

struct Multiply
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT T operator()(const T &a, const T &b) const
  {
    return T(a * b);
  }
};

struct Divide
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT T operator()(const T &a, const T &b) const
  {
    return T(a / b);
  }
};

struct Negate
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT T operator()(const T &x) const
  {
    return T(-x);
  }
};

#if defined(VTKM_GCC) || defined(VTKM_CLANG)
#pragma GCC diagnostic pop
#endif // gcc || clang

} // namespace internal

//-----------------------------------------------------------------------------

// Pre declaration
template<typename T, vtkm::IdComponent Size> class Vec;

namespace detail {

/// Base implementation of all Vec classes.
///
template<typename T, vtkm::IdComponent Size, typename DerivedClass>
class VecBase
{
public:
  typedef T ComponentType;
  static const vtkm::IdComponent NUM_COMPONENTS=Size;

protected:
  VTKM_EXEC_CONT_EXPORT
  VecBase() {}

  VTKM_EXEC_CONT_EXPORT
  explicit VecBase(const ComponentType& value)
  {
    vtkm::internal::AssignScalarToVec<NUM_COMPONENTS>()(
          this->Components, value);
  }

  template<typename OtherValueType, typename OtherDerivedType>
  VTKM_EXEC_CONT_EXPORT
  VecBase(const VecBase<OtherValueType,Size,OtherDerivedType> &src)
  {
    vtkm::internal::VecCopy<ComponentType,NUM_COMPONENTS>()(
      this->Components, src);
  }

public:
  VTKM_EXEC_CONT_EXPORT
  vtkm::IdComponent GetNumberOfComponents() { return NUM_COMPONENTS; }

  template<vtkm::IdComponent OtherSize>
  VTKM_EXEC_CONT_EXPORT
  void CopyInto(vtkm::Vec<ComponentType,OtherSize> &dest) const
  {
    for (vtkm::IdComponent index = 0;
         (index < NUM_COMPONENTS) && (index < OtherSize);
         index++)
    {
      dest[index] = (*this)[index];
    }
  }

  VTKM_EXEC_CONT_EXPORT
  DerivedClass &operator=(const DerivedClass &src)
  {
    vtkm::internal::VecCopy<ComponentType,NUM_COMPONENTS>()(
      this->Components, src);
    return *reinterpret_cast<DerivedClass *>(this);
  }

  VTKM_EXEC_CONT_EXPORT
  const ComponentType &operator[](vtkm::IdComponent idx) const
  {
    return this->Components[idx];
  }
  VTKM_EXEC_CONT_EXPORT
  ComponentType &operator[](vtkm::IdComponent idx)
  {
    return this->Components[idx];
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator==(const DerivedClass &other) const
  {
    return vtkm::internal::VecEquals<NUM_COMPONENTS>()(
          *reinterpret_cast<const DerivedClass*>(this), other);
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator<(const DerivedClass &other) const
  {
    for(vtkm::IdComponent componentIndex = 0;
        componentIndex < NUM_COMPONENTS;
        ++componentIndex)
    {
      //ignore equals as that represents check next value
      if(this->Components[componentIndex] < other[componentIndex])
      {
        return true;
      }
      else if(other[componentIndex] < this->Components[componentIndex])
      {
        return false;
      }
    } //if all same we are not less

    return false;
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator!=(const DerivedClass &other) const
  {
    return !(this->operator==(other));
  }

  VTKM_EXEC_CONT_EXPORT
  ComponentType Dot(const DerivedClass &other) const
  {
    ComponentType result = this->Components[0]*other[0];
    for (vtkm::IdComponent componentIndex = 1;
         componentIndex < Size;
         componentIndex++)
    {
      result += this->Components[componentIndex]*other[componentIndex];
    }
    return result;
  }

  VTKM_EXEC_CONT_EXPORT
  DerivedClass operator+(const DerivedClass &other) const
  {
    return vtkm::internal::VecComponentWiseBinaryOperation<Size>()(
          *reinterpret_cast<const DerivedClass*>(this),
          other,
          vtkm::internal::Add());
  }

  VTKM_EXEC_CONT_EXPORT
  DerivedClass operator-(const DerivedClass &other) const
  {
    return vtkm::internal::VecComponentWiseBinaryOperation<Size>()(
          *reinterpret_cast<const DerivedClass*>(this),
          other,
          vtkm::internal::Subtract());
  }

  VTKM_EXEC_CONT_EXPORT
  DerivedClass operator*(const DerivedClass &other) const
  {
    return vtkm::internal::VecComponentWiseBinaryOperation<Size>()(
          *reinterpret_cast<const DerivedClass*>(this),
          other,
          vtkm::internal::Multiply());
  }

  VTKM_EXEC_CONT_EXPORT
  DerivedClass operator*(ComponentType scalar) const
  {
    return vtkm::internal::VecComponentWiseUnaryOperation<Size>()(
          *reinterpret_cast<const DerivedClass*>(this),
          vtkm::internal::BindRightBinaryOp<
            ComponentType,vtkm::internal::Multiply>(scalar));
  }


  VTKM_EXEC_CONT_EXPORT
  DerivedClass operator/(const DerivedClass &other) const
  {
    return vtkm::internal::VecComponentWiseBinaryOperation<Size>()(
          *reinterpret_cast<const DerivedClass*>(this),
          other,
          vtkm::internal::Divide());
  }

protected:
  ComponentType Components[NUM_COMPONENTS];
};

} // namespace detail

//-----------------------------------------------------------------------------

/// \brief A short fixed-length array.
///
/// The \c Vec templated class holds a short array of values of a size and
/// type specified by the template arguments.
///
/// The \c Vec class is most often used to represent vectors in the
/// mathematical sense as a quantity with a magnitude and direction. Vectors
/// are, of course, used extensively in computational geometry as well as
/// phyiscal simulations. The \c Vec class can be (and is) repurposed for more
/// general usage of holding a fixed-length sequence of objects.
///
/// There is no real limit to the size of the sequence (other than the largest
/// number representable by vtkm::IdComponent), but the \c Vec class is really
/// designed for small sequences (seldom more than 10).
///
template<typename T, vtkm::IdComponent Size>
class Vec : public detail::VecBase<T, Size, Vec<T,Size> >
{
  typedef detail::VecBase<T, Size, Vec<T,Size> > Superclass;
public:
#ifdef VTKM_DOXYGEN_ONLY
  typedef T ComponentType;
  static const vtkm::IdComponent NUM_COMPONENTS=Size;
#endif

  VTKM_EXEC_CONT_EXPORT Vec() {}
  VTKM_EXEC_CONT_EXPORT explicit Vec(const T& value) : Superclass(value) {  }
  // VTKM_EXEC_CONT_EXPORT explicit Vec(const T* values) : Superclass(values) {  }

  template<typename OtherType>
  VTKM_EXEC_CONT_EXPORT
  Vec(const Vec<OtherType, Size> &src) : Superclass(src) {  }
};

//-----------------------------------------------------------------------------
// Specializations for common small tuples. We implement them a bit specially.

// A vector of size 0 cannot use VecBase because it will try to create a
// zero length array which troubles compilers. Vecs of size 0 are a bit
// pointless but might occur in some generic functions or classes.
template<typename T>
class Vec<T, 0>
{
public:
  typedef T ComponentType;
  static const vtkm::IdComponent NUM_COMPONENTS = 0;

  VTKM_EXEC_CONT_EXPORT Vec() {}
  VTKM_EXEC_CONT_EXPORT explicit Vec(const ComponentType&) {  }

  template<typename OtherType>
  VTKM_EXEC_CONT_EXPORT Vec(const Vec<OtherType, NUM_COMPONENTS> &) {  }

  VTKM_EXEC_CONT_EXPORT
  Vec<ComponentType, NUM_COMPONENTS> &
  operator=(const Vec<ComponentType, NUM_COMPONENTS> &)
  {
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT
  ComponentType operator[](vtkm::IdComponent vtkmNotUsed(idx)) const
  {
    return ComponentType();
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator==(const Vec<T, NUM_COMPONENTS> &vtkmNotUsed(other)) const
  {
    return true;
  }
  VTKM_EXEC_CONT_EXPORT
  bool operator!=(const Vec<T, NUM_COMPONENTS> &vtkmNotUsed(other)) const
  {
      return false;
  }
};

//-----------------------------------------------------------------------------
// Specializations for common tuple sizes (with special names).

template<typename T>
class Vec<T,2> : public detail::VecBase<T, 2, Vec<T,2> >
{
  typedef detail::VecBase<T, 2, Vec<T,2> > Superclass;

public:
  VTKM_EXEC_CONT_EXPORT Vec() {}
  VTKM_EXEC_CONT_EXPORT explicit Vec(const T& value) : Superclass(value) {  }

  template<typename OtherType>
  VTKM_EXEC_CONT_EXPORT Vec(const Vec<OtherType, 2> &src) : Superclass(src) {  }

  VTKM_EXEC_CONT_EXPORT
  Vec(const T &x, const T &y)
  {
    this->Components[0] = x;
    this->Components[1] = y;
  }
};

/// Id2 corresponds to a 2-dimensional index
typedef vtkm::Vec<vtkm::Id,2> Id2;


template<typename T>
class Vec<T,3> : public detail::VecBase<T, 3, Vec<T,3> >
{
  typedef detail::VecBase<T, 3, Vec<T,3> > Superclass;
public:
  VTKM_EXEC_CONT_EXPORT Vec() {}
  VTKM_EXEC_CONT_EXPORT explicit Vec(const T& value) : Superclass(value) {  }

  template<typename OtherType>
  VTKM_EXEC_CONT_EXPORT Vec(const Vec<OtherType, 3> &src) : Superclass(src) {  }

  VTKM_EXEC_CONT_EXPORT
  Vec(const T &x, const T &y, const T &z)
  {
    this->Components[0] = x;
    this->Components[1] = y;
    this->Components[2] = z;
  }
};

/// Id3 corresponds to a 3-dimensional index for 3d arrays.  Note that
/// the precision of each index may be less than vtkm::Id.
typedef vtkm::Vec<vtkm::Id,3> Id3;


template<typename T>
class Vec<T,4> : public detail::VecBase<T, 4, Vec<T,4> >
{
  typedef detail::VecBase<T, 4, Vec<T,4> > Superclass;
public:
  VTKM_EXEC_CONT_EXPORT Vec() {}
  VTKM_EXEC_CONT_EXPORT explicit Vec(const T& value) : Superclass(value) {  }

  template<typename OtherType>
  VTKM_EXEC_CONT_EXPORT Vec(const Vec<OtherType, 4> &src) : Superclass(src) {  }

  VTKM_EXEC_CONT_EXPORT
  Vec(const T &x, const T &y, const T &z, const T &w)
  {
    this->Components[0] = x;
    this->Components[1] = y;
    this->Components[2] = z;
    this->Components[3] = w;
  }
};


/// Initializes and returns a Vec of length 2.
///
template<typename T>
VTKM_EXEC_CONT_EXPORT
vtkm::Vec<T,2> make_Vec(const T &x, const T &y)
{
  return vtkm::Vec<T,2>(x, y);
}

/// Initializes and returns a Vec of length 3.
///
template<typename T>
VTKM_EXEC_CONT_EXPORT
vtkm::Vec<T,3> make_Vec(const T &x, const T &y, const T &z)
{
  return vtkm::Vec<T,3>(x, y, z);
}

/// Initializes and returns a Vec of length 4.
///
template<typename T>
VTKM_EXEC_CONT_EXPORT
vtkm::Vec<T,4> make_Vec(const T &x, const T &y, const T &z, const T &w)
{
  return vtkm::Vec<T,4>(x, y, z, w);
}

template<typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT_EXPORT
T dot(const vtkm::Vec<T,Size> &a, const vtkm::Vec<T,Size> &b)
{
  T result = T(a[0]*b[0]);
  for (vtkm::IdComponent componentIndex = 1; componentIndex < Size; componentIndex++)
  {
    result = T(result + a[componentIndex]*b[componentIndex]);
  }
  return result;
}

template<typename T>
VTKM_EXEC_CONT_EXPORT
T dot(const vtkm::Vec<T,2> &a, const vtkm::Vec<T,2> &b)
{
  return T((a[0]*b[0]) + (a[1]*b[1]));
}

template<typename T>
VTKM_EXEC_CONT_EXPORT
T dot(const vtkm::Vec<T,3> &a, const vtkm::Vec<T,3> &b)
{
  return T((a[0]*b[0]) + (a[1]*b[1]) + (a[2]*b[2]));
}

template<typename T>
VTKM_EXEC_CONT_EXPORT
T dot(const vtkm::Vec<T,4> &a, const vtkm::Vec<T,4> &b)
{
  return T((a[0]*b[0]) + (a[1]*b[1]) + (a[2]*b[2]) + (a[3]*b[3]));
}

//Integer types of a width less than an integer get implicitly casted to
//an integer when doing a multiplication.
#define VTK_M_INTEGER_PROMOTION_SCALAR_DOT(type) \
  VTKM_EXEC_CONT_EXPORT type dot(type a, type b) { return static_cast<type>(a * b); }
VTK_M_INTEGER_PROMOTION_SCALAR_DOT(vtkm::Int8)
VTK_M_INTEGER_PROMOTION_SCALAR_DOT(vtkm::UInt8)
VTK_M_INTEGER_PROMOTION_SCALAR_DOT(vtkm::Int16)
VTK_M_INTEGER_PROMOTION_SCALAR_DOT(vtkm::UInt16)
#define VTK_M_SCALAR_DOT(type) \
  VTKM_EXEC_CONT_EXPORT type dot(type a, type b) { return a * b; }
VTK_M_SCALAR_DOT(vtkm::Int32)
VTK_M_SCALAR_DOT(vtkm::UInt32)
VTK_M_SCALAR_DOT(vtkm::Int64)
VTK_M_SCALAR_DOT(vtkm::UInt64)
VTK_M_SCALAR_DOT(vtkm::Float32)
VTK_M_SCALAR_DOT(vtkm::Float64)

} // End of namespace vtkm

// Declared outside of vtkm namespace so that the operator works with all code.

template<typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT_EXPORT
vtkm::Vec<T, Size> operator*(T scalar, const vtkm::Vec<T, Size> &vec)
{
  return vtkm::internal::VecComponentWiseUnaryOperation<Size>()(
        vec,
        vtkm::internal::BindLeftBinaryOp<T,vtkm::internal::Multiply>(scalar));
}

// The enable_if for this operator is effectively disabling the negate
// operator for Vec of unsigned integers. Another approach would be
// to use disable_if<is_unsigned>. That would be more inclusive but would
// also allow other types like Vec<Vec<unsigned> >. If necessary, we could
// change this implementation to be more inclusive.
template<typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT_EXPORT
typename boost::enable_if<
  typename boost::mpl::or_<
    typename boost::is_floating_point<T>::type,
    typename boost::is_signed<T>::type>::type,
  vtkm::Vec<T,Size> >::type
operator-(const vtkm::Vec<T,Size> &x)
{
  return vtkm::internal::VecComponentWiseUnaryOperation<Size>()(
        x, vtkm::internal::Negate());
}

#endif //vtk_m_Types_h
