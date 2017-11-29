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
#ifndef vtk_m_Types_h
#define vtk_m_Types_h

#include <vtkm/internal/Configure.h>
#include <vtkm/internal/ExportMacros.h>

#include <vtkm/Assert.h>
#include <vtkm/StaticAssert.h>

#include <iostream>
#include <type_traits>

/*!
 * \namespace vtkm
 * \brief VTK-m Toolkit.
 *
 * vtkm is the namespace for the VTK-m Toolkit. It contains other sub namespaces,
 * as well as basic data types and functions callable from all components in VTK-m
 * toolkit.
 *
 * \namespace vtkm::cont
 * \brief VTK-m Control Environment.
 *
 * vtkm::cont defines the publicly accessible API for the VTK-m Control
 * Environment. Users of the VTK-m Toolkit can use this namespace to access the
 * Control Environment.
 *
 * \namespace vtkm::cont::arg
 * \brief Transportation controls for Control Environment Objects.
 *
 * vtkm::cont::arg includes the classes that allows the vtkm::worklet::Dispatchers
 * to request Control Environment Objects to be transfered to the Execution Environment.
 *
 * \namespace vtkm::cont::cuda
 * \brief CUDA implementation for Control Environment.
 *
 * vtkm::cont::cuda includes the code to implement the VTK-m Control Environment
 * for the CUDA-based device adapter.
 *
 * \namespace vtkm::cont::serial
 * \brief Serial implementation for Control Environment.
 *
 * vtkm::cont::serial includes the code to implement the VTK-m Control Environment
 * for the serial device adapter.
 *
 * \namespace vtkm::cont::tbb
 * \brief TBB implementation for Control Environment.
 *
 * vtkm::cont::tbb includes the code to implement the VTK-m Control Environment
 * for the TBB-based device adapter.
 *
 * \namespace vtkm::exec
 * \brief VTK-m Execution Environment.
 *
 * vtkm::exec defines the publicly accessible API for the VTK-m Execution
 * Environment. Worklets typically use classes/apis defined within this
 * namespace alone.
 *
 * \namespace vtkm::exec::cuda
 * \brief CUDA implementation for Execution Environment.
 *
 * vtkm::exec::cuda includes the code to implement the VTK-m Execution Environment
 * for the CUDA-based device adapter.
 *
 * \namespace vtkm::exec::serial
 * \brief CUDA implementation for Execution Environment.
 *
 * vtkm::exec::serial includes the code to implement the VTK-m Execution Environment
 * for the serial device adapter.
 *
 * \namespace vtkm::exec::tbb
 * \brief TBB implementation for Execution Environment.
 *
 * vtkm::exec::tbb includes the code to implement the VTK-m Execution Environment
 * for the TBB device adapter.
 *
 * \namespace vtkm::filter
 * \brief VTK-m Filters
 *
 * vtkm::filter is the collection of predefined filters that take data as input
 * and write new data as output. Filters operate on vtkm::cont::DataSet objects,
 * vtkm::cont::Fields, and other runtime typeless objects.
 *
 * \namespace vtkm::internal
 * \brief VTK-m Internal Environment
 *
 * vtkm::internal defines API which is internal and subject to frequent
 * change. This should not be used for projects using VTK-m. Instead it servers
 * are a reference for the developers of VTK-m.
 *
 * \namespace vtkm::interop
 * \brief VTK-m OpenGL Interoperability
 *
 * vtkm::interop defines the publicly accessible API for interoperability between
 * vtkm and OpenGL.
 *
 * \namespace vtkm::io
 * \brief VTK-m IO
 *
 * vtkm::io defines API for basic reading of VTK files. Intended to be used for
 * examples and testing.
 *
 * \namespace vtkm::rendering
 * \brief VTK-m Rendering
 *
 * vtkm::rendering defines API for
 *
 * \namespace vtkm::testing
 * \brief Internal testing classes
 *
 * \namespace vtkm::worklet
 * \brief VTK-m Worklets
 *
 * vtkm::worklet defines API for the low level worklets that operate on an element of data,
 * and the dispatcher that execute them in the execution environment.
 *
 * VTK-m provides numerous worklet implementations. These worklet implementations for the most
 * part provide the underlying implementations of the algorithms in vtkm::filter.
 *
 */

namespace vtkm
{
//*****************************************************************************
// Typedefs for basic types.
//*****************************************************************************

#if VTKM_SIZE_FLOAT == 4
using Float32 = float;
#else
#error Could not find a 32-bit float.
#endif

#if VTKM_SIZE_DOUBLE == 8
using Float64 = double;
#else
#error Could not find a 64-bit float.
#endif

#if VTKM_SIZE_CHAR == 1
using Int8 = signed char;
using UInt8 = unsigned char;
#else
#error Could not find an 8-bit integer.
#endif

#if VTKM_SIZE_SHORT == 2
using Int16 = short;
using UInt16 = unsigned short;
#else
#error Could not find a 16-bit integer.
#endif

#if VTKM_SIZE_INT == 4
using Int32 = int;
using UInt32 = unsigned int;
#else
#error Could not find a 32-bit integer.
#endif

//In this order so that we exactly match the logic that exists in VTK
#if VTKM_SIZE_LONG_LONG == 8
using Int64 = long long;
using UInt64 = unsigned long long;
#elif VTKM_SIZE_LONG == 8
using Int64 = signed long;
using UInt64 = unsigned long;
#else
#error Could not find a 64-bit integer.
#endif

//-----------------------------------------------------------------------------

#if VTKM_SIZE_ID == 4

/// Represents an ID (index into arrays).
using Id = vtkm::Int32;

#elif VTKM_SIZE_ID == 8

/// Represents an ID.
using Id = vtkm::Int64;

#else
#error Unknown Id Size
#endif

/// Represents a component ID (index of component in a vector). The number
/// of components, being a value fixed at compile time, is generally assumed
/// to be quite small. However, we are currently using a 32-bit width
/// integer because modern processors tend to access them more efficiently
/// than smaller widths.
using IdComponent = vtkm::Int32;

#ifdef VTKM_USE_DOUBLE_PRECISION

/// The floating point type to use when no other precision is specified.
using FloatDefault = vtkm::Float64;

#else //VTKM_USE_DOUBLE_PRECISION

/// The floating point type to use when no other precision is specified.
using FloatDefault = vtkm::Float32;

#endif //VTKM_USE_DOUBLE_PRECISION

namespace internal
{

//-----------------------------------------------------------------------------

/// Placeholder class for when a type is not applicable.
///
struct NullType
{
};

//-----------------------------------------------------------------------------
template <vtkm::IdComponent Size>
struct VecComponentWiseUnaryOperation
{
  template <typename T, typename UnaryOpType>
  VTKM_EXEC_CONT T operator()(const T& v, const UnaryOpType& unaryOp) const
  {
    T result;
    for (vtkm::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = unaryOp(v[i]);
    }
    return result;
  }
};

template <>
struct VecComponentWiseUnaryOperation<1>
{
  template <typename T, typename UnaryOpType>
  VTKM_EXEC_CONT T operator()(const T& v, const UnaryOpType& unaryOp) const
  {
    return T(unaryOp(v[0]));
  }
};

template <>
struct VecComponentWiseUnaryOperation<2>
{
  template <typename T, typename UnaryOpType>
  VTKM_EXEC_CONT T operator()(const T& v, const UnaryOpType& unaryOp) const
  {
    return T(unaryOp(v[0]), unaryOp(v[1]));
  }
};

template <>
struct VecComponentWiseUnaryOperation<3>
{
  template <typename T, typename UnaryOpType>
  VTKM_EXEC_CONT T operator()(const T& v, const UnaryOpType& unaryOp) const
  {
    return T(unaryOp(v[0]), unaryOp(v[1]), unaryOp(v[2]));
  }
};

template <>
struct VecComponentWiseUnaryOperation<4>
{
  template <typename T, typename UnaryOpType>
  VTKM_EXEC_CONT T operator()(const T& v, const UnaryOpType& unaryOp) const
  {
    return T(unaryOp(v[0]), unaryOp(v[1]), unaryOp(v[2]), unaryOp(v[3]));
  }
};

template <typename T, typename BinaryOpType, typename ReturnT = T>
struct BindLeftBinaryOp
{
  // Warning: a reference.
  const T& LeftValue;
  const BinaryOpType BinaryOp;
  VTKM_EXEC_CONT
  BindLeftBinaryOp(const T& leftValue, BinaryOpType binaryOp = BinaryOpType())
    : LeftValue(leftValue)
    , BinaryOp(binaryOp)
  {
  }

  template <typename RightT>
  VTKM_EXEC_CONT ReturnT operator()(const RightT& rightValue) const
  {
    return static_cast<ReturnT>(this->BinaryOp(this->LeftValue, static_cast<T>(rightValue)));
  }

private:
  void operator=(const BindLeftBinaryOp<T, BinaryOpType, ReturnT>&) = delete;
};

template <typename T, typename BinaryOpType, typename ReturnT = T>
struct BindRightBinaryOp
{
  // Warning: a reference.
  const T& RightValue;
  const BinaryOpType BinaryOp;
  VTKM_EXEC_CONT
  BindRightBinaryOp(const T& rightValue, BinaryOpType binaryOp = BinaryOpType())
    : RightValue(rightValue)
    , BinaryOp(binaryOp)
  {
  }

  template <typename LeftT>
  VTKM_EXEC_CONT ReturnT operator()(const LeftT& leftValue) const
  {
    return static_cast<ReturnT>(this->BinaryOp(static_cast<T>(leftValue), this->RightValue));
  }

private:
  void operator=(const BindRightBinaryOp<T, BinaryOpType, ReturnT>&) = delete;
};

} // namespace internal

// Disable conversion warnings for Add, Subtract, Multiply, Divide on GCC only.
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang
struct Add
{
  template <typename T>
  VTKM_EXEC_CONT T operator()(const T& a, const T& b) const
  {
    return T(a + b);
  }
};

struct Subtract
{
  template <typename T>
  VTKM_EXEC_CONT T operator()(const T& a, const T& b) const
  {
    return T(a - b);
  }
};

struct Multiply
{
  template <typename T>
  VTKM_EXEC_CONT T operator()(const T& a, const T& b) const
  {
    return T(a * b);
  }
};

struct Divide
{
  template <typename T>
  VTKM_EXEC_CONT T operator()(const T& a, const T& b) const
  {
    return T(a / b);
  }
};

struct Negate
{
  template <typename T>
  VTKM_EXEC_CONT T operator()(const T& x) const
  {
    return T(-x);
  }
};

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang

//-----------------------------------------------------------------------------

// Pre declaration
template <typename T, vtkm::IdComponent Size>
class VTKM_ALWAYS_EXPORT Vec;

template <typename T>
class VTKM_ALWAYS_EXPORT VecC;

template <typename T>
class VTKM_ALWAYS_EXPORT VecCConst;

namespace detail
{

/// Base implementation of all Vec and VecC classes.
///
// Disable conversion warnings for Add, Subtract, Multiply, Divide on GCC only.
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
//
// NVCC 7.5 and below does not recognize this pragma inside of class bodies,
// so put them before entering the class.
//
#if (defined(VTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8))
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang
#endif // use cuda < 8
template <typename T, typename DerivedClass>
class VTKM_ALWAYS_EXPORT VecBaseCommon
{
public:
  using ComponentType = T;

protected:
  VTKM_EXEC_CONT
  VecBaseCommon() = default;

  VTKM_EXEC_CONT
  const DerivedClass& Derived() const { return *static_cast<const DerivedClass*>(this); }

  VTKM_EXEC_CONT
  DerivedClass& Derived() { return *static_cast<DerivedClass*>(this); }

private:
  // Only for internal use
  VTKM_EXEC_CONT
  vtkm::IdComponent NumComponents() const { return this->Derived().GetNumberOfComponents(); }

  // Only for internal use
  VTKM_EXEC_CONT
  const T& Component(vtkm::IdComponent index) const { return this->Derived()[index]; }

  // Only for internal use
  VTKM_EXEC_CONT
  T& Component(vtkm::IdComponent index) { return this->Derived()[index]; }

public:
  template <vtkm::IdComponent OtherSize>
  VTKM_EXEC_CONT void CopyInto(vtkm::Vec<ComponentType, OtherSize>& dest) const
  {
    for (vtkm::IdComponent index = 0; (index < this->NumComponents()) && (index < OtherSize);
         index++)
    {
      dest[index] = this->Component(index);
    }
  }

  template <typename OtherComponentType, typename OtherVecType>
  VTKM_EXEC_CONT DerivedClass& operator=(
    const vtkm::detail::VecBaseCommon<OtherComponentType, OtherVecType>& src)
  {
    const OtherVecType& srcDerived = static_cast<const OtherVecType&>(src);
    VTKM_ASSERT(this->NumComponents() == srcDerived.GetNumberOfComponents());
    for (vtkm::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) = OtherComponentType(srcDerived[i]);
    }
    return this->Derived();
  }

  VTKM_EXEC_CONT
  bool operator==(const DerivedClass& other) const
  {
    bool equal = true;
    for (vtkm::IdComponent i = 0; i < this->NumComponents() && equal; ++i)
    {
      equal = (this->Component(i) == other[i]);
    }
    return equal;
  }

  VTKM_EXEC_CONT
  bool operator<(const DerivedClass& other) const
  {
    for (vtkm::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      // ignore equals as that represents check next value
      if (this->Component(i) < other[i])
      {
        return true;
      }
      else if (other[i] < this->Component(i))
      {
        return false;
      }
    } // if all same we are not less

    return false;
  }

  VTKM_EXEC_CONT
  bool operator!=(const DerivedClass& other) const { return !(this->operator==(other)); }

  VTKM_EXEC_CONT
  ComponentType Dot(const VecBaseCommon<ComponentType, DerivedClass>& other) const
  {
    // Why the static_cast here and below? Because * on small integers (char,
    // short) promotes the result to a 32-bit int. After helpfully promoting
    // the width of the result, some compilers then warn you about casting it
    // back to the type you were expecting in the first place. The static_cast
    // suppresses this warning.
    ComponentType result = static_cast<ComponentType>(this->Component(0) * other.Component(0));
    for (vtkm::IdComponent i = 1; i < this->NumComponents(); ++i)
    {
      result = static_cast<ComponentType>(result + this->Component(i) * other.Component(i));
    }
    return result;
  }

#if (!(defined(VTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang
#endif // not using cuda < 8

  template <vtkm::IdComponent Size>
  VTKM_EXEC_CONT vtkm::Vec<ComponentType, Size> operator+(
    const vtkm::Vec<ComponentType, Size>& other) const
  {
    VTKM_ASSERT(Size == this->NumComponents());
    vtkm::Vec<ComponentType, Size> result;
    for (vtkm::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = this->Component(i) + other[i];
    }
    return result;
  }

  template <typename OtherClass>
  VTKM_EXEC_CONT DerivedClass& operator+=(const VecBaseCommon<ComponentType, OtherClass>& other)
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    VTKM_ASSERT(this->NumComponents() == other_derived.GetNumberOfComponents());
    for (vtkm::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) += other_derived[i];
    }
    return this->Derived();
  }

  template <vtkm::IdComponent Size>
  VTKM_EXEC_CONT vtkm::Vec<ComponentType, Size> operator-(
    const vtkm::Vec<ComponentType, Size>& other) const
  {
    VTKM_ASSERT(Size == this->NumComponents());
    vtkm::Vec<ComponentType, Size> result;
    for (vtkm::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = this->Component(i) - other[i];
    }
    return result;
  }

  template <typename OtherClass>
  VTKM_EXEC_CONT DerivedClass& operator-=(const VecBaseCommon<ComponentType, OtherClass>& other)
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    VTKM_ASSERT(this->NumComponents() == other_derived.GetNumberOfComponents());
    for (vtkm::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) -= other_derived[i];
    }
    return this->Derived();
  }

  template <vtkm::IdComponent Size>
  VTKM_EXEC_CONT vtkm::Vec<ComponentType, Size> operator*(
    const vtkm::Vec<ComponentType, Size>& other) const
  {
    vtkm::Vec<ComponentType, Size> result;
    for (vtkm::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = this->Component(i) * other[i];
    }
    return result;
  }

  template <typename OtherClass>
  VTKM_EXEC_CONT DerivedClass& operator*=(const VecBaseCommon<ComponentType, OtherClass>& other)
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    VTKM_ASSERT(this->NumComponents() == other_derived.GetNumberOfComponents());
    for (vtkm::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) *= other_derived[i];
    }
    return this->Derived();
  }

  template <vtkm::IdComponent Size>
  VTKM_EXEC_CONT vtkm::Vec<ComponentType, Size> operator/(
    const vtkm::Vec<ComponentType, Size>& other) const
  {
    vtkm::Vec<ComponentType, Size> result;
    for (vtkm::IdComponent i = 0; i < Size; ++i)
    {
      result[i] = this->Component(i) / other[i];
    }
    return result;
  }

  template <typename OtherClass>
  VTKM_EXEC_CONT DerivedClass& operator/=(const VecBaseCommon<ComponentType, OtherClass>& other)
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    VTKM_ASSERT(this->NumComponents() == other_derived.GetNumberOfComponents());
    for (vtkm::IdComponent i = 0; i < this->NumComponents(); ++i)
    {
      this->Component(i) /= other_derived[i];
    }
    return this->Derived();
  }

#if (!(defined(VTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang
#endif // not using cuda < 8

  VTKM_EXEC_CONT
  ComponentType* GetPointer() { return &this->Component(0); }

  VTKM_EXEC_CONT
  const ComponentType* GetPointer() const { return &this->Component(0); }
};
#if (defined(VTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8))
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang
#endif // use cuda < 8

/// Base implementation of all Vec classes.
///
template <typename T, vtkm::IdComponent Size, typename DerivedClass>
class VTKM_ALWAYS_EXPORT VecBase : public vtkm::detail::VecBaseCommon<T, DerivedClass>
{
public:
  using ComponentType = T;
  static const vtkm::IdComponent NUM_COMPONENTS = Size;

protected:
  VTKM_EXEC_CONT
  VecBase() = default;

  VTKM_EXEC_CONT
  explicit VecBase(const ComponentType& value)
  {
    for (vtkm::IdComponent i = 0; i < Size; ++i)
    {
      this->Components[i] = value;
    }
  }

  template <typename OtherValueType, typename OtherDerivedType>
  VTKM_EXEC_CONT VecBase(const VecBase<OtherValueType, Size, OtherDerivedType>& src)
  {
    for (vtkm::IdComponent i = 0; i < Size; ++i)
    {
      this->Components[i] = static_cast<T>(src[i]);
    }
  }

public:
  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfComponents() const { return NUM_COMPONENTS; }

  VTKM_EXEC_CONT
  //DRP
  /*  inline */ const ComponentType& operator[](vtkm::IdComponent idx) const
  {
    VTKM_ASSERT(idx >= 0);
    VTKM_ASSERT(idx < NUM_COMPONENTS);
    return this->Components[idx];
  }
  VTKM_EXEC_CONT
  //DRP
  /*inline*/ ComponentType& operator[](vtkm::IdComponent idx)
  {
    VTKM_ASSERT(idx >= 0);
    VTKM_ASSERT(idx < NUM_COMPONENTS);
    return this->Components[idx];
  }

#if (!(defined(VTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang
#endif // not using cuda < 8

  template <typename OtherComponentType, typename OtherClass>
  VTKM_EXEC_CONT DerivedClass
  operator+(const VecBaseCommon<OtherComponentType, OtherClass>& other) const
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    VTKM_ASSERT(NUM_COMPONENTS == other_derived.GetNumberOfComponents());

    DerivedClass result;
    for (vtkm::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = this->Components[i] + static_cast<ComponentType>(other_derived[i]);
    }
    return result;
  }

  template <typename OtherComponentType, typename OtherClass>
  VTKM_EXEC_CONT DerivedClass
  operator-(const VecBaseCommon<OtherComponentType, OtherClass>& other) const
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    VTKM_ASSERT(NUM_COMPONENTS == other_derived.GetNumberOfComponents());

    DerivedClass result;
    for (vtkm::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = this->Components[i] - static_cast<ComponentType>(other_derived[i]);
    }
    return result;
  }

  template <typename OtherComponentType, typename OtherClass>
  VTKM_EXEC_CONT DerivedClass
  operator*(const VecBaseCommon<OtherComponentType, OtherClass>& other) const
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    VTKM_ASSERT(NUM_COMPONENTS == other_derived.GetNumberOfComponents());

    DerivedClass result;
    for (vtkm::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = this->Components[i] * static_cast<ComponentType>(other_derived[i]);
    }
    return result;
  }

  template <typename OtherComponentType, typename OtherClass>
  VTKM_EXEC_CONT DerivedClass
  operator/(const VecBaseCommon<OtherComponentType, OtherClass>& other) const
  {
    const OtherClass& other_derived = static_cast<const OtherClass&>(other);
    VTKM_ASSERT(NUM_COMPONENTS == other_derived.GetNumberOfComponents());

    DerivedClass result;
    for (vtkm::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      result[i] = this->Components[i] / static_cast<ComponentType>(other_derived[i]);
    }
    return result;
  }

#if (!(defined(VTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang
#endif // not using cuda < 8

protected:
  ComponentType Components[NUM_COMPONENTS];
};

/// Base of all VecC and VecCConst classes.
///
template <typename T, typename DerivedClass>
class VTKM_ALWAYS_EXPORT VecCBase : public vtkm::detail::VecBaseCommon<T, DerivedClass>
{
protected:
  VTKM_EXEC_CONT
  VecCBase() {}
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
template <typename T, vtkm::IdComponent Size>
class VTKM_ALWAYS_EXPORT Vec : public detail::VecBase<T, Size, Vec<T, Size>>
{
  using Superclass = detail::VecBase<T, Size, Vec<T, Size>>;

public:
#ifdef VTKM_DOXYGEN_ONLY
  using ComponentType = T;
  static const vtkm::IdComponent NUM_COMPONENTS = Size;
#endif

  VTKM_EXEC_CONT Vec() = default;
  VTKM_EXEC_CONT explicit Vec(const T& value)
    : Superclass(value)
  {
  }
  // VTKM_EXEC_CONT explicit Vec(const T* values) : Superclass(values) {  }

  template <typename OtherType>
  VTKM_EXEC_CONT Vec(const Vec<OtherType, Size>& src)
    : Superclass(src)
  {
  }
};

//-----------------------------------------------------------------------------
// Specializations for common small tuples. We implement them a bit specially.

// A vector of size 0 cannot use VecBase because it will try to create a
// zero length array which troubles compilers. Vecs of size 0 are a bit
// pointless but might occur in some generic functions or classes.
template <typename T>
class VTKM_ALWAYS_EXPORT Vec<T, 0>
{
public:
  using ComponentType = T;
  static const vtkm::IdComponent NUM_COMPONENTS = 0;

  VTKM_EXEC_CONT Vec() = default;
  VTKM_EXEC_CONT explicit Vec(const ComponentType&) {}

  template <typename OtherType>
  VTKM_EXEC_CONT Vec(const Vec<OtherType, NUM_COMPONENTS>&)
  {
  }

  VTKM_EXEC_CONT
  Vec<ComponentType, NUM_COMPONENTS>& operator=(const Vec<ComponentType, NUM_COMPONENTS>&)
  {
    return *this;
  }

  VTKM_EXEC_CONT
  ComponentType operator[](vtkm::IdComponent vtkmNotUsed(idx)) const { return ComponentType(); }

  VTKM_EXEC_CONT
  bool operator==(const Vec<T, NUM_COMPONENTS>& vtkmNotUsed(other)) const { return true; }
  VTKM_EXEC_CONT
  bool operator!=(const Vec<T, NUM_COMPONENTS>& vtkmNotUsed(other)) const { return false; }
};

// Vectors of size 1 should implicitly convert between the scalar and the
// vector. Otherwise, it should behave the same.
template <typename T>
class VTKM_ALWAYS_EXPORT Vec<T, 1> : public detail::VecBase<T, 1, Vec<T, 1>>
{
  using Superclass = detail::VecBase<T, 1, Vec<T, 1>>;

public:
  VTKM_EXEC_CONT Vec() = default;
  VTKM_EXEC_CONT explicit Vec(const T& value)
    : Superclass(value)
  {
  }

  template <typename OtherType>
  VTKM_EXEC_CONT Vec(const Vec<OtherType, 1>& src)
    : Superclass(src)
  {
  }

  // This convenience operator removed because it was causing ambiguous
  // overload errors
  //  VTKM_EXEC_CONT
  //  operator T() const
  //  {
  //    return this->Components[0];
  //  }
};

//-----------------------------------------------------------------------------
// Specializations for common tuple sizes (with special names).

template <typename T>
class VTKM_ALWAYS_EXPORT Vec<T, 2> : public detail::VecBase<T, 2, Vec<T, 2>>
{
  using Superclass = detail::VecBase<T, 2, Vec<T, 2>>;

public:
  VTKM_EXEC_CONT Vec() = default;
  VTKM_EXEC_CONT explicit Vec(const T& value)
    : Superclass(value)
  {
  }

  template <typename OtherType>
  VTKM_EXEC_CONT Vec(const Vec<OtherType, 2>& src)
    : Superclass(src)
  {
  }

  VTKM_EXEC_CONT
  Vec(const T& x, const T& y)
  {
    this->Components[0] = x;
    this->Components[1] = y;
  }
};

/// Id2 corresponds to a 2-dimensional index
using Id2 = vtkm::Vec<vtkm::Id, 2>;

template <typename T>
class VTKM_ALWAYS_EXPORT Vec<T, 3> : public detail::VecBase<T, 3, Vec<T, 3>>
{
  using Superclass = detail::VecBase<T, 3, Vec<T, 3>>;

public:
  VTKM_EXEC_CONT Vec() = default;
  VTKM_EXEC_CONT explicit Vec(const T& value)
    : Superclass(value)
  {
  }

  template <typename OtherType>
  VTKM_EXEC_CONT Vec(const Vec<OtherType, 3>& src)
    : Superclass(src)
  {
  }

  VTKM_EXEC_CONT
  Vec(const T& x, const T& y, const T& z)
  {
    this->Components[0] = x;
    this->Components[1] = y;
    this->Components[2] = z;
  }
};

/// Id3 corresponds to a 3-dimensional index for 3d arrays.  Note that
/// the precision of each index may be less than vtkm::Id.
using Id3 = vtkm::Vec<vtkm::Id, 3>;

template <typename T>
class VTKM_ALWAYS_EXPORT Vec<T, 4> : public detail::VecBase<T, 4, Vec<T, 4>>
{
  using Superclass = detail::VecBase<T, 4, Vec<T, 4>>;

public:
  VTKM_EXEC_CONT Vec() = default;
  VTKM_EXEC_CONT explicit Vec(const T& value)
    : Superclass(value)
  {
  }

  template <typename OtherType>
  VTKM_EXEC_CONT Vec(const Vec<OtherType, 4>& src)
    : Superclass(src)
  {
  }

  VTKM_EXEC_CONT
  Vec(const T& x, const T& y, const T& z, const T& w)
  {
    this->Components[0] = x;
    this->Components[1] = y;
    this->Components[2] = z;
    this->Components[3] = w;
  }
};

/// Provides the appropriate type when not sure if using a Vec or a scalar in a
/// templated class or function. The \c Type in the struct is the same as the
/// \c ComponentType when \c NumComponents is 1 and a \c Vec otherwise.
///
template <typename ComponentType, vtkm::IdComponent NumComponents>
struct VecOrScalar
{
  using Type = vtkm::Vec<ComponentType, NumComponents>;
};
template <typename ComponentType>
struct VecOrScalar<ComponentType, 1>
{
  using Type = ComponentType;
};

/// Initializes and returns a Vec of length 2.
///
template <typename T>
VTKM_EXEC_CONT vtkm::Vec<T, 2> make_Vec(const T& x, const T& y)
{
  return vtkm::Vec<T, 2>(x, y);
}

/// Initializes and returns a Vec of length 3.
///
template <typename T>
VTKM_EXEC_CONT vtkm::Vec<T, 3> make_Vec(const T& x, const T& y, const T& z)
{
  return vtkm::Vec<T, 3>(x, y, z);
}

/// Initializes and returns a Vec of length 4.
///
template <typename T>
VTKM_EXEC_CONT vtkm::Vec<T, 4> make_Vec(const T& x, const T& y, const T& z, const T& w)
{
  return vtkm::Vec<T, 4>(x, y, z, w);
}

/// \brief A Vec-like representation for short arrays.
///
/// The \c VecC class takes a short array of values and provides an interface
/// that mimics \c Vec. This provides a mechanism to treat C arrays like a \c
/// Vec. It is useful in situations where you want to use a \c Vec but the data
/// must come from elsewhere or in certain situations where the size cannot be
/// determined at compile time. In particular, \c Vec objects of different
/// sizes can potentially all be converted to a \c VecC of the same type.
///
/// Note that \c VecC holds a reference to an outside array given to it. If
/// that array gets destroyed (for example because the source goes out of
/// scope), the behavior becomes undefined.
///
/// You cannot use \c VecC with a const type in its template argument. For
/// example, you cannot declare <tt>VecC<const vtkm::Id></tt>. If you want a
/// non-mutable \c VecC, the \c VecCConst class (e.g.
/// <tt>VecCConst<vtkm::Id></tt>).
///
template <typename T>
class VTKM_ALWAYS_EXPORT VecC : public detail::VecCBase<T, VecC<T>>
{
  using Superclass = detail::VecCBase<T, VecC<T>>;

  VTKM_STATIC_ASSERT_MSG(std::is_const<T>::value == false,
                         "You cannot use VecC with a const type as its template argument. "
                         "Use either const VecC or VecCConst.");

public:
#ifdef VTKM_DOXYGEN_ONLY
  using ComponentType = T;
#endif

  VTKM_EXEC_CONT
  VecC()
    : Components(nullptr)
    , NumberOfComponents(0)
  {
  }

  VTKM_EXEC_CONT
  VecC(T* array, vtkm::IdComponent size)
    : Components(array)
    , NumberOfComponents(size)
  {
  }

  template <vtkm::IdComponent Size>
  VTKM_EXEC_CONT VecC(vtkm::Vec<T, Size>& src)
    : Components(src.GetPointer())
    , NumberOfComponents(Size)
  {
  }

  VTKM_EXEC_CONT
  explicit VecC(T& src)
    : Components(&src)
    , NumberOfComponents(1)
  {
  }

  VTKM_EXEC_CONT
  VecC(const VecC<T>& src)
    : Components(src.Components)
    , NumberOfComponents(src.NumberOfComponents)
  {
  }

  VTKM_EXEC_CONT
  const T& operator[](vtkm::IdComponent index) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->NumberOfComponents);
    return this->Components[index];
  }

  VTKM_EXEC_CONT
  T& operator[](vtkm::IdComponent index)
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->NumberOfComponents);
    return this->Components[index];
  }

  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfComponents() const { return this->NumberOfComponents; }

  VTKM_EXEC_CONT
  VecC<T>& operator=(const VecC<T>& src)
  {
    VTKM_ASSERT(this->NumberOfComponents == src.GetNumberOfComponents());
    for (vtkm::IdComponent index = 0; index < this->NumberOfComponents; index++)
    {
      (*this)[index] = src[index];
    }

    return *this;
  }

private:
  T* const Components;
  vtkm::IdComponent NumberOfComponents;
};

/// \brief A const version of VecC
///
/// \c VecCConst is a non-mutable form of \c VecC. It can be used in place of
/// \c VecC when a constant array is available.
///
/// A \c VecC can be automatically converted to a \c VecCConst, but not vice
/// versa, so function arguments should use \c VecCConst when the data do not
/// need to be changed.
///
template <typename T>
class VTKM_ALWAYS_EXPORT VecCConst : public detail::VecCBase<T, VecCConst<T>>
{
  using Superclass = detail::VecCBase<T, VecCConst<T>>;

  VTKM_STATIC_ASSERT_MSG(std::is_const<T>::value == false,
                         "You cannot use VecCConst with a const type as its template argument. "
                         "Remove the const from the type.");

public:
#ifdef VTKM_DOXYGEN_ONLY
  using ComponentType = T;
#endif

  VTKM_EXEC_CONT
  VecCConst()
    : Components(nullptr)
    , NumberOfComponents(0)
  {
  }

  VTKM_EXEC_CONT
  VecCConst(const T* array, vtkm::IdComponent size)
    : Components(array)
    , NumberOfComponents(size)
  {
  }

  template <vtkm::IdComponent Size>
  VTKM_EXEC_CONT VecCConst(const vtkm::Vec<T, Size>& src)
    : Components(src.GetPointer())
    , NumberOfComponents(Size)
  {
  }

  VTKM_EXEC_CONT
  explicit VecCConst(const T& src)
    : Components(&src)
    , NumberOfComponents(1)
  {
  }

  VTKM_EXEC_CONT
  VecCConst(const VecCConst<T>& src)
    : Components(src.Components)
    , NumberOfComponents(src.NumberOfComponents)
  {
  }

  VTKM_EXEC_CONT
  VecCConst(const VecC<T>& src)
    : Components(src.Components)
    , NumberOfComponents(src.NumberOfComponents)
  {
  }

  VTKM_EXEC_CONT
  const T& operator[](vtkm::IdComponent index) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->NumberOfComponents);
    return this->Components[index];
  }

  VTKM_EXEC_CONT
  vtkm::IdComponent GetNumberOfComponents() const { return this->NumberOfComponents; }

private:
  const T* const Components;
  vtkm::IdComponent NumberOfComponents;

  // You are not allowed to assign to a VecCConst, so these operators are not
  // implemented and are disallowed.
  void operator=(const VecCConst<T>&) = delete;
  void operator+=(const VecCConst<T>&) = delete;
  void operator-=(const VecCConst<T>&) = delete;
  void operator*=(const VecCConst<T>&) = delete;
  void operator/=(const VecCConst<T>&) = delete;
};

/// Creates a \c VecC from an input array.
///
template <typename T>
VTKM_EXEC_CONT static inline vtkm::VecC<T> make_VecC(T* array, vtkm::IdComponent size)
{
  return vtkm::VecC<T>(array, size);
}

/// Creates a \c VecCConst from a constant input array.
///
template <typename T>
VTKM_EXEC_CONT static inline vtkm::VecCConst<T> make_VecC(const T* array, vtkm::IdComponent size)
{
  return vtkm::VecCConst<T>(array, size);
}

// A pre-declaration of vtkm::Pair so that classes templated on them can refer
// to it. The actual implementation is in vtkm/Pair.h.
template <typename U, typename V>
struct Pair;

template <typename T, vtkm::IdComponent Size>
static inline VTKM_EXEC_CONT T dot(const vtkm::Vec<T, Size>& a, const vtkm::Vec<T, Size>& b)
{
  T result = T(a[0] * b[0]);
  for (vtkm::IdComponent i = 1; i < Size; ++i)
  {
    result = T(result + a[i] * b[i]);
  }
  return result;
}

template <typename T>
static inline VTKM_EXEC_CONT T dot(const vtkm::Vec<T, 2>& a, const vtkm::Vec<T, 2>& b)
{
  return T((a[0] * b[0]) + (a[1] * b[1]));
}

template <typename T>
static inline VTKM_EXEC_CONT T dot(const vtkm::Vec<T, 3>& a, const vtkm::Vec<T, 3>& b)
{
  return T((a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]));
}

template <typename T>
static inline VTKM_EXEC_CONT T dot(const vtkm::Vec<T, 4>& a, const vtkm::Vec<T, 4>& b)
{
  return T((a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]) + (a[3] * b[3]));
}

template <typename T, typename VecType>
static inline VTKM_EXEC_CONT T dot(const vtkm::detail::VecBaseCommon<T, VecType>& a,
                                   const vtkm::detail::VecBaseCommon<T, VecType>& b)
{
  return a.Dot(b);
}

template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT T ReduceSum(const vtkm::Vec<T, Size>& a)
{
  T result = a[0];
  for (vtkm::IdComponent i = 1; i < Size; ++i)
  {
    result += a[i];
  }
  return result;
}

template <typename T>
VTKM_EXEC_CONT T ReduceSum(const vtkm::Vec<T, 2>& a)
{
  return a[0] + a[1];
}

template <typename T>
VTKM_EXEC_CONT T ReduceSum(const vtkm::Vec<T, 3>& a)
{
  return a[0] + a[1] + a[2];
}

template <typename T>
VTKM_EXEC_CONT T ReduceSum(const vtkm::Vec<T, 4>& a)
{
  return a[0] + a[1] + a[2] + a[3];
}

template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT T ReduceProduct(const vtkm::Vec<T, Size>& a)
{
  T result = a[0];
  for (vtkm::IdComponent i = 1; i < Size; ++i)
  {
    result *= a[i];
  }
  return result;
}

template <typename T>
VTKM_EXEC_CONT T ReduceProduct(const vtkm::Vec<T, 2>& a)
{
  return a[0] * a[1];
}

template <typename T>
VTKM_EXEC_CONT T ReduceProduct(const vtkm::Vec<T, 3>& a)
{
  return a[0] * a[1] * a[2];
}

template <typename T>
VTKM_EXEC_CONT T ReduceProduct(const vtkm::Vec<T, 4>& a)
{
  return a[0] * a[1] * a[2] * a[3];
}

// Integer types of a width less than an integer get implicitly casted to
// an integer when doing a multiplication.
#define VTK_M_INTEGER_PROMOTION_SCALAR_DOT(type)                                                   \
  static inline VTKM_EXEC_CONT type dot(type a, type b) { return static_cast<type>(a * b); }
VTK_M_INTEGER_PROMOTION_SCALAR_DOT(vtkm::Int8)
VTK_M_INTEGER_PROMOTION_SCALAR_DOT(vtkm::UInt8)
VTK_M_INTEGER_PROMOTION_SCALAR_DOT(vtkm::Int16)
VTK_M_INTEGER_PROMOTION_SCALAR_DOT(vtkm::UInt16)
#define VTK_M_SCALAR_DOT(type)                                                                     \
  static inline VTKM_EXEC_CONT type dot(type a, type b) { return a * b; }
VTK_M_SCALAR_DOT(vtkm::Int32)
VTK_M_SCALAR_DOT(vtkm::UInt32)
VTK_M_SCALAR_DOT(vtkm::Int64)
VTK_M_SCALAR_DOT(vtkm::UInt64)
VTK_M_SCALAR_DOT(vtkm::Float32)
VTK_M_SCALAR_DOT(vtkm::Float64)

} // End of namespace vtkm

// Declared outside of vtkm namespace so that the operator works with all code.

template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Vec<T, Size> operator*(T scalar, const vtkm::Vec<T, Size>& vec)
{
  return vtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, vtkm::internal::BindLeftBinaryOp<T, vtkm::Multiply>(scalar));
}

template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Vec<T, Size> operator*(const vtkm::Vec<T, Size>& vec, T scalar)
{
  return vtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, vtkm::internal::BindRightBinaryOp<T, vtkm::Multiply>(scalar));
}

template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Vec<T, Size> operator*(vtkm::Float64 scalar, const vtkm::Vec<T, Size>& vec)
{
  return vtkm::Vec<T, Size>(vtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, vtkm::internal::BindLeftBinaryOp<vtkm::Float64, vtkm::Multiply, T>(scalar)));
}

template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Vec<T, Size> operator*(const vtkm::Vec<T, Size>& vec, vtkm::Float64 scalar)
{
  return vtkm::Vec<T, Size>(vtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, vtkm::internal::BindRightBinaryOp<vtkm::Float64, vtkm::Multiply, T>(scalar)));
}

template <vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Vec<vtkm::Float64, Size> operator*(vtkm::Float64 scalar,
                                                        const vtkm::Vec<vtkm::Float64, Size>& vec)
{
  return vtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, vtkm::internal::BindLeftBinaryOp<vtkm::Float64, vtkm::Multiply>(scalar));
}

template <vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Vec<vtkm::Float64, Size> operator*(const vtkm::Vec<vtkm::Float64, Size>& vec,
                                                        vtkm::Float64 scalar)
{
  return vtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, vtkm::internal::BindRightBinaryOp<vtkm::Float64, vtkm::Multiply>(scalar));
}

template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Vec<T, Size> operator/(const vtkm::Vec<T, Size>& vec, T scalar)
{
  return vtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, vtkm::internal::BindRightBinaryOp<T, vtkm::Divide>(scalar));
}

template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Vec<T, Size> operator/(const vtkm::Vec<T, Size>& vec, vtkm::Float64 scalar)
{
  return vtkm::Vec<T, Size>(vtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, vtkm::internal::BindRightBinaryOp<vtkm::Float64, vtkm::Divide, T>(scalar)));
}

template <vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Vec<vtkm::Float64, Size> operator/(const vtkm::Vec<vtkm::Float64, Size>& vec,
                                                        vtkm::Float64 scalar)
{
  return vtkm::internal::VecComponentWiseUnaryOperation<Size>()(
    vec, vtkm::internal::BindRightBinaryOp<vtkm::Float64, vtkm::Divide>(scalar));
}

// clang-format off
// The enable_if for this operator is effectively disabling the negate
// operator for Vec of unsigned integers. Another approach would be
// to use enable_if<!is_unsigned>. That would be more inclusive but would
// also allow other types like Vec<Vec<unsigned> >. If necessary, we could
// change this implementation to be more inclusive.
template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT
typename std::enable_if<(std::is_floating_point<T>::value || std::is_signed<T>::value),
                        vtkm::Vec<T, Size>>::type
operator-(const vtkm::Vec<T, Size>& x)
{
  return vtkm::internal::VecComponentWiseUnaryOperation<Size>()(x, vtkm::Negate());
}
// clang-format on

/// Helper function for printing out vectors during testing.
///
template <typename T, vtkm::IdComponent Size>
VTKM_CONT std::ostream& operator<<(std::ostream& stream, const vtkm::Vec<T, Size>& vec)
{
  stream << "[";
  for (vtkm::IdComponent component = 0; component < Size - 1; component++)
  {
    stream << vec[component] << ",";
  }
  return stream << vec[Size - 1] << "]";
}

/// Helper function for printing out pairs during testing.
///
template <typename T, typename U>
VTKM_EXEC_CONT std::ostream& operator<<(std::ostream& stream, const vtkm::Pair<T, U>& vec)
{
  return stream << "[" << vec.first << "," << vec.second << "]";
}

#endif //vtk_m_Types_h
