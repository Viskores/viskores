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
//  Copyright 2014. Los Alamos National Security
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

namespace internal {

#if VTKM_SIZE_INT == 4
typedef int Int32Type;
typedef unsigned int UInt32Type;
#else
#error Could not find a 32-bit integer.
#endif

#if VTKM_SIZE_LONG == 8
typedef long Int64Type;
typedef unsigned long UInt64Type;
#elif VTKM_SIZE_LONG_LONG == 8
typedef long long Int64Type;
typedef unsigned long long UInt64Type;
#else
#error Could not find a 64-bit integer.
#endif

//-----------------------------------------------------------------------------

template<int Size>
struct equals
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a, const T& b) const
  {
    return equals<Size-1>()(a,b) && a[Size-1] == b[Size-1];
  }
};

template<>
struct equals<1>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a, const T& b) const
  {
    return a[0] == b[0];
  }
};

template<>
struct equals<2>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a, const T& b) const
  {
    return a[0] == b[0] && a[1] == b[1];
  }
};

template<>
struct equals<3>
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a, const T& b) const
  {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
  }
};

template<int Size>
struct assign_scalar_to_vector
{
  template<typename VectorType, typename ComponentType>
  VTKM_EXEC_CONT_EXPORT
  void operator()(VectorType &dest, const ComponentType &src)
  {
    assign_scalar_to_vector<Size-1>()(dest, src);
    dest[Size-1] = src;
  }
};

template<>
struct assign_scalar_to_vector<1>
{
  template<typename VectorType, typename ComponentType>
  VTKM_EXEC_CONT_EXPORT
  void operator()(VectorType &dest, const ComponentType &src)
  {
    dest[0] = src;
  }
};

template<>
struct assign_scalar_to_vector<2>
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
struct assign_scalar_to_vector<3>
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

template<int Size>
struct copy_vector
{
  template<typename T1, typename T2>
  VTKM_EXEC_CONT_EXPORT void operator()(T1 &dest, const T2 &src)
  {
    copy_vector<Size-1>()(dest, src);
    dest[Size-1] = src[Size-1];
  }
};

template<>
struct copy_vector<1>
{
  template<typename T1, typename T2>
  VTKM_EXEC_CONT_EXPORT void operator()(T1 &dest, const T2 &src)
  {
    dest[0] = src[0];
  }
};

template<>
struct copy_vector<2>
{
  template<typename T1, typename T2>
  VTKM_EXEC_CONT_EXPORT void operator()(T1 &dest, const T2 &src)
  {
    dest[0] = src[0];
    dest[1] = src[1];
  }
};

template<>
struct copy_vector<3>
{
  template<typename T1, typename T2>
  VTKM_EXEC_CONT_EXPORT void operator()(T1 &dest, const T2 &src)
  {
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
  }
};


} // namespace internal

//-----------------------------------------------------------------------------

#if VTKM_SIZE_ID == 4

/// Represents an ID.
typedef internal::Int32Type Id;

#elif VTKM_SIZE_ID == 8

/// Represents an ID.
typedef internal::Int64Type Id;

#else
#error Unknown Id Size
#endif

#ifdef VTKM_USE_DOUBLE_PRECISION

/// Scalar corresponds to a floating point number.
typedef double Scalar;

#else //VTKM_USE_DOUBLE_PRECISION

/// Scalar corresponds to a floating point number.
typedef float Scalar;

#endif //VTKM_USE_DOUBLE_PRECISION

//-----------------------------------------------------------------------------

/// Tuple corresponds to a Size-tuple of type T
template<typename T, int Size>
class Tuple
{
public:
  typedef T ComponentType;
  static const int NUM_COMPONENTS=Size;

  VTKM_EXEC_CONT_EXPORT Tuple() {}
  VTKM_EXEC_CONT_EXPORT explicit Tuple(const ComponentType& value)
  {
    for(int i=0; i < NUM_COMPONENTS; ++i)
    {
      this->Components[i]=value;
    }
  }
  VTKM_EXEC_CONT_EXPORT explicit Tuple(const ComponentType* values)
  {
    for(int i=0; i < NUM_COMPONENTS; ++i)
    {
      this->Components[i]=values[i];
    }
  }
  VTKM_EXEC_CONT_EXPORT
  Tuple(const Tuple<ComponentType, Size> &src)
  {
    for (int i = 0; i < NUM_COMPONENTS; i++)
    {
      this->Components[i] = src[i];
    }
  }

  VTKM_EXEC_CONT_EXPORT
  Tuple<ComponentType, Size> &operator=(const Tuple<ComponentType, Size> &src)
  {
    for (int i = 0; i < NUM_COMPONENTS; i++)
    {
      this->Components[i] = src[i];
    }
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT const ComponentType &operator[](int idx) const
  {
    return this->Components[idx];
  }
  VTKM_EXEC_CONT_EXPORT ComponentType &operator[](int idx)
  {
    return this->Components[idx];
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator==(const Tuple<T,NUM_COMPONENTS> &other) const
  {
    bool same = true;
    for (int componentIndex=0; componentIndex<NUM_COMPONENTS; componentIndex++)
    {
      same &= (this->Components[componentIndex] == other[componentIndex]);
    }
    return same;
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator<(const Tuple<T,NUM_COMPONENTS> &other) const
  {
    for(vtkm::Id i=0; i < NUM_COMPONENTS; ++i)
    {
      //ignore equals as that represents check next value
      if(this->Components[i] < other[i])
      { return true; }
      else if(other[i] < this->Components[i])
      { return false; }
    } //if all same we are not less
    return false;
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator!=(const Tuple<T,NUM_COMPONENTS> &other) const
  {
    return !(this->operator==(other));
  }

protected:
  ComponentType Components[NUM_COMPONENTS];
};

//-----------------------------------------------------------------------------
// Specializations for common tuple sizes (with special names).

template<typename T>
class Tuple<T,2>
{
public:
  typedef T ComponentType;
  static const int NUM_COMPONENTS = 2;

  VTKM_EXEC_CONT_EXPORT Tuple() {}
  VTKM_EXEC_CONT_EXPORT explicit Tuple(const ComponentType& value)
  {
    internal::assign_scalar_to_vector<NUM_COMPONENTS>()(this->Components,value);
  }
  VTKM_EXEC_CONT_EXPORT explicit Tuple(const ComponentType* values)
  {
    internal::copy_vector<NUM_COMPONENTS>()(this->Components, values);
  }
  VTKM_EXEC_CONT_EXPORT Tuple(ComponentType x, ComponentType y)
  {
    this->Components[0] = x;
    this->Components[1] = y;
  }
  VTKM_EXEC_CONT_EXPORT
  Tuple(const Tuple<ComponentType, NUM_COMPONENTS> &src)
  {
    internal::copy_vector<NUM_COMPONENTS>()(this->Components, src.Components);
  }

  VTKM_EXEC_CONT_EXPORT
  Tuple<ComponentType, NUM_COMPONENTS> &
  operator=(const Tuple<ComponentType, NUM_COMPONENTS> &src)
  {
    internal::copy_vector<NUM_COMPONENTS>()(this->Components, src.Components);
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT const ComponentType &operator[](int idx) const
  {
    return this->Components[idx];
  }
  VTKM_EXEC_CONT_EXPORT ComponentType &operator[](int idx)
  {
    return this->Components[idx];
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator==(const Tuple<T,NUM_COMPONENTS> &other) const
  {
    return internal::equals<NUM_COMPONENTS>()(*this, other);
  }
  VTKM_EXEC_CONT_EXPORT
  bool operator!=(const Tuple<T,NUM_COMPONENTS> &other) const
  {
    return !(this->operator==(other));
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator<(const Tuple<T,NUM_COMPONENTS> &other) const
  {
    return( (this->Components[0] < other[0])  ||
            (!(other[0] < this->Components[0]) && (this->Components[1] < other[1]))
          );
  }

protected:
  ComponentType Components[NUM_COMPONENTS];
};

/// Vector2 corresponds to a 2-tuple
typedef vtkm::Tuple<vtkm::Scalar,2> Vector2;


/// Id2 corresponds to a 2-dimensional index
typedef vtkm::Tuple<vtkm::Id,2> Id2;

template<typename T>
class Tuple<T,3>
{
public:
  typedef T ComponentType;
  static const int NUM_COMPONENTS = 3;

  VTKM_EXEC_CONT_EXPORT Tuple() {}
  VTKM_EXEC_CONT_EXPORT explicit Tuple(const ComponentType& value)
  {
    internal::assign_scalar_to_vector<NUM_COMPONENTS>()(this->Components,value);
  }
  VTKM_EXEC_CONT_EXPORT explicit Tuple(const ComponentType* values)
  {
    internal::copy_vector<NUM_COMPONENTS>()(this->Components, values);
  }
  VTKM_EXEC_CONT_EXPORT
  Tuple(ComponentType x, ComponentType y, ComponentType z)
  {
    this->Components[0] = x;
    this->Components[1] = y;
    this->Components[2] = z;
  }
  VTKM_EXEC_CONT_EXPORT
  Tuple(const Tuple<ComponentType, NUM_COMPONENTS> &src)
  {
    internal::copy_vector<NUM_COMPONENTS>()(this->Components, src.Components);
  }

  VTKM_EXEC_CONT_EXPORT
  Tuple<ComponentType, NUM_COMPONENTS> &
  operator=(const Tuple<ComponentType, NUM_COMPONENTS> &src)
  {
    internal::copy_vector<NUM_COMPONENTS>()(this->Components, src.Components);
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT const ComponentType &operator[](int idx) const
  {
    return this->Components[idx];
  }
  VTKM_EXEC_CONT_EXPORT ComponentType &operator[](int idx)
  {
    return this->Components[idx];
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator==(const Tuple<T,NUM_COMPONENTS> &other) const
  {
    return internal::equals<NUM_COMPONENTS>()(*this, other);
  }
  VTKM_EXEC_CONT_EXPORT
  bool operator!=(const Tuple<T,NUM_COMPONENTS> &other) const
  {
    return !(this->operator==(other));
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator<(const Tuple<T,NUM_COMPONENTS> &other) const
  {
    return((this->Components[0] < other[0])    ||
           ( !(other[0] < this->Components[0]) &&
             (this->Components[1] < other[1]))  ||
           ( !(other[0] < this->Components[0]) &&
             !(other[1] < this->Components[1]) &&
             (this->Components[2] < other[2]) ) );
  }

protected:
  ComponentType Components[NUM_COMPONENTS];
};

/// Vector3 corresponds to a 3-tuple
typedef vtkm::Tuple<vtkm::Scalar,3> Vector3;

/// Id3 corresponds to a 3-dimensional index for 3d arrays.  Note that
/// the precision of each index may be less than vtkm::Id.
typedef vtkm::Tuple<vtkm::Id,3> Id3;

template<typename T>
class Tuple<T,4>
{
public:
  typedef T ComponentType;
  static const int NUM_COMPONENTS = 4;

  VTKM_EXEC_CONT_EXPORT Tuple() {}
  VTKM_EXEC_CONT_EXPORT explicit Tuple(const ComponentType& value)
  {
    internal::assign_scalar_to_vector<NUM_COMPONENTS>()(this->Components,value);
  }
  VTKM_EXEC_CONT_EXPORT explicit Tuple(const ComponentType* values)
  {
    internal::copy_vector<NUM_COMPONENTS>()(this->Components, values);
  }
  VTKM_EXEC_CONT_EXPORT
  Tuple(ComponentType x, ComponentType y, ComponentType z, ComponentType w)
  {
    this->Components[0] = x;
    this->Components[1] = y;
    this->Components[2] = z;
    this->Components[3] = w;
  }
  VTKM_EXEC_CONT_EXPORT
  Tuple(const Tuple<ComponentType, NUM_COMPONENTS> &src)
  {
    internal::copy_vector<NUM_COMPONENTS>()(this->Components, src.Components);
  }

  VTKM_EXEC_CONT_EXPORT
  Tuple<ComponentType, NUM_COMPONENTS> &
  operator=(const Tuple<ComponentType, NUM_COMPONENTS> &src)
  {
    internal::copy_vector<NUM_COMPONENTS>()(this->Components, src.Components);
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT const ComponentType &operator[](int idx) const
  {
    return this->Components[idx];
  }
  VTKM_EXEC_CONT_EXPORT ComponentType &operator[](int idx)
  {
    return this->Components[idx];
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator==(const Tuple<T,NUM_COMPONENTS> &other) const
  {
    return internal::equals<NUM_COMPONENTS>()(*this, other);
  }
  VTKM_EXEC_CONT_EXPORT
  bool operator!=(const Tuple<T,NUM_COMPONENTS> &other) const
  {
    return !(this->operator==(other));
  }

  VTKM_EXEC_CONT_EXPORT
  bool operator<(const Tuple<T,NUM_COMPONENTS> &other) const
  {
    return((this->Components[0] < other[0])       ||
           ( !(other[0] < this->Components[0])    &&
             this->Components[1] < other[1])  ||
           ( !(other[0] < this->Components[0])    &&
             !(other[1] < this->Components[1])    &&
             (this->Components[2] < other[2]) )   ||
           ( !(other[0] < this->Components[0])    &&
             !(other[1] < this->Components[1])    &&
             !(other[2] < this->Components[2])    &&
             (this->Components[3] < other[3])) );
  }

protected:
  ComponentType Components[NUM_COMPONENTS];
};

/// Vector4 corresponds to a 4-tuple
typedef vtkm::Tuple<vtkm::Scalar,4> Vector4;


/// Initializes and returns a Vector2.
VTKM_EXEC_CONT_EXPORT vtkm::Vector2 make_Vector2(vtkm::Scalar x,
                                               vtkm::Scalar y)
{
  return vtkm::Vector2(x, y);
}

/// Initializes and returns a Vector3.
VTKM_EXEC_CONT_EXPORT vtkm::Vector3 make_Vector3(vtkm::Scalar x,
                                               vtkm::Scalar y,
                                               vtkm::Scalar z)
{
  return vtkm::Vector3(x, y, z);
}

/// Initializes and returns a Vector4.
VTKM_EXEC_CONT_EXPORT vtkm::Vector4 make_Vector4(vtkm::Scalar x,
                                               vtkm::Scalar y,
                                               vtkm::Scalar z,
                                               vtkm::Scalar w)
{
  return vtkm::Vector4(x, y, z, w);
}

/// Initializes and returns an Id3
VTKM_EXEC_CONT_EXPORT vtkm::Id3 make_Id3(vtkm::Id x, vtkm::Id y, vtkm::Id z)
{
  return vtkm::Id3(x, y, z);
}

template<typename T, int Size>
VTKM_EXEC_CONT_EXPORT T dot(const vtkm::Tuple<T,Size> &a,
                            const vtkm::Tuple<T,Size> &b)
{
  T result = a[0]*b[0];
  for (int componentIndex = 1; componentIndex < Size; componentIndex++)
  {
    result += a[componentIndex]*b[componentIndex];
  }
  return result;
}

VTKM_EXEC_CONT_EXPORT vtkm::Id dot(vtkm::Id a, vtkm::Id b)
{
  return a * b;
}

VTKM_EXEC_CONT_EXPORT vtkm::Scalar dot(vtkm::Scalar a, vtkm::Scalar b)
{
  return a * b;
}

} // End of namespace vtkm

template<typename T, int Size>
VTKM_EXEC_CONT_EXPORT vtkm::Tuple<T,Size> operator+(const vtkm::Tuple<T,Size> &a,
                                                  const vtkm::Tuple<T,Size> &b)
{
  vtkm::Tuple<T,Size> result;
  for (int componentIndex = 0; componentIndex < Size; componentIndex++)
  {
    result[componentIndex] = a[componentIndex] + b[componentIndex];
  }
  return result;
}
template<typename T, int Size>
VTKM_EXEC_CONT_EXPORT vtkm::Tuple<T,Size> operator-(const vtkm::Tuple<T,Size> &a,
                                                  const vtkm::Tuple<T,Size> &b)
{
  vtkm::Tuple<T,Size> result;
  for (int componentIndex = 0; componentIndex < Size; componentIndex++)
  {
    result[componentIndex] = a[componentIndex] - b[componentIndex];
  }
  return result;
}
template<typename T, int Size>
VTKM_EXEC_CONT_EXPORT vtkm::Tuple<T,Size> operator*(const vtkm::Tuple<T,Size> &a,
                                                  const vtkm::Tuple<T,Size> &b)
{
  vtkm::Tuple<T,Size> result;
  for (int componentIndex = 0; componentIndex < Size; componentIndex++)
  {
    result[componentIndex] = a[componentIndex] * b[componentIndex];
  }
  return result;
}
template<typename T, int Size>
VTKM_EXEC_CONT_EXPORT vtkm::Tuple<T,Size> operator/(const vtkm::Tuple<T,Size> &a,
                                                  const vtkm::Tuple<T,Size> &b)
{
  vtkm::Tuple<T,Size> result;
  for (int componentIndex = 0; componentIndex < Size; componentIndex++)
  {
    result[componentIndex] = a[componentIndex] / b[componentIndex];
  }
  return result;
}

template<typename Ta, typename Tb, int Size>
VTKM_EXEC_CONT_EXPORT vtkm::Tuple<Ta,Size> operator*(const vtkm::Tuple<Ta,Size> &a,
                                                   const Tb &b)
{
  vtkm::Tuple<Ta,Size> result;
  for (int componentIndex = 0; componentIndex < Size; componentIndex++)
  {
    result[componentIndex] = a[componentIndex] * b;
  }
  return result;
}
template<typename Ta, typename Tb, int Size>
VTKM_EXEC_CONT_EXPORT vtkm::Tuple<Tb,Size> operator*(const Ta &a,
                                                   const vtkm::Tuple<Tb,Size> &b)
{
  vtkm::Tuple<Tb,Size> result;
  for (int componentIndex = 0; componentIndex < Size; componentIndex++)
  {
    result[componentIndex] = a * b[componentIndex];
  }
  return result;
}

#endif //vtkm_Types_h
