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
#ifndef vtk_m_TypeTraits_h
#define vtk_m_TypeTraits_h

#include <vtkm/Types.h>

namespace vtkm {

/// Tag used to identify types that aren't Real, Integer, Scalar or Vector.
///
struct TypeTraitsUnknownTag {};

/// Tag used to identify types that store real (floating-point) numbers. A
/// TypeTraits class will typedef this class to NumericTag if it stores real
/// numbers (or vectors of real numbers).
///
struct TypeTraitsRealTag {};

/// Tag used to identify types that store integer numbers. A TypeTraits class
/// will typedef this class to NumericTag if it stores integer numbers (or
/// vectors of integers).
///
struct TypeTraitsIntegerTag {};

/// Tag used to identify 0 dimensional types (scalars). Scalars can also be
/// treated like vectors when used with VecTraits. A TypeTraits class will
/// typedef this class to DimensionalityTag.
///
struct TypeTraitsScalarTag {};

/// Tag used to identify 1 dimensional types (vectors). A TypeTraits class will
/// typedef this class to DimensionalityTag.
///
struct TypeTraitsVectorTag {};


/// The TypeTraits class provides helpful compile-time information about the
/// basic types used in VTKm (and a few others for convienience). The majority
/// of TypeTraits contents are typedefs to tags that can be used to easily
/// override behavior of called functions.
///
template<typename T>
class TypeTraits
{
public:
  /// \brief A tag to determing whether the type is integer or real.
  ///
  /// This tag is either TypeTraitsRealTag or TypeTraitsIntegerTag.
  typedef TypeTraitsUnknownTag NumericTag;

  /// \brief A tag to determine whether the type has multiple components.
  ///
  /// This tag is either TypeTraitsScalarTag or TypeTraitsVectorTag. Scalars can
  /// also be treated as vectors.
  typedef TypeTraitsUnknownTag DimensionalityTag;

  VTKM_EXEC_CONT_EXPORT static T ZeroInitialization() { return T(); }
};

// Const types should have the same traits as their non-const counterparts.
//
template<typename T>
struct TypeTraits<const T> : TypeTraits<T>
{  };

#define VTKM_BASIC_REAL_TYPE(T) \
  template<> struct TypeTraits<T> { \
    typedef TypeTraitsRealTag NumericTag; \
    typedef TypeTraitsScalarTag DimensionalityTag; \
    VTKM_EXEC_CONT_EXPORT static T ZeroInitialization() { return T(); } \
  };

#define VTKM_BASIC_INTEGER_TYPE(T) \
  template<> struct TypeTraits< T > { \
    typedef TypeTraitsIntegerTag NumericTag; \
    typedef TypeTraitsScalarTag DimensionalityTag; \
    VTKM_EXEC_CONT_EXPORT static T ZeroInitialization() \
      { \
      typedef T ReturnType; \
      return ReturnType(); \
      } \
  }; \

/// Traits for basic C++ types.
///

VTKM_BASIC_REAL_TYPE(float)
VTKM_BASIC_REAL_TYPE(double)

VTKM_BASIC_INTEGER_TYPE(char)
VTKM_BASIC_INTEGER_TYPE(signed char)
VTKM_BASIC_INTEGER_TYPE(unsigned char)
VTKM_BASIC_INTEGER_TYPE(short)
VTKM_BASIC_INTEGER_TYPE(unsigned short)
VTKM_BASIC_INTEGER_TYPE(int)
VTKM_BASIC_INTEGER_TYPE(unsigned int)
VTKM_BASIC_INTEGER_TYPE(long)
VTKM_BASIC_INTEGER_TYPE(unsigned long)
VTKM_BASIC_INTEGER_TYPE(long long)
VTKM_BASIC_INTEGER_TYPE(unsigned long long)


#undef VTKM_BASIC_REAL_TYPE
#undef VTKM_BASIC_INTEGER_TYPE

/// Traits for Vec types.
///
template<typename T, vtkm::IdComponent Size>
struct TypeTraits<vtkm::Vec<T,Size> >
{
  typedef typename vtkm::TypeTraits<T>::NumericTag NumericTag;
  typedef TypeTraitsVectorTag DimensionalityTag;

  VTKM_EXEC_CONT_EXPORT
  static vtkm::Vec<T,Size> ZeroInitialization()
    { return vtkm::Vec<T,Size>( (T()) ); }
};

/// \brief Traits for Pair types.
///
template<typename T, typename U>
struct TypeTraits<vtkm::Pair<T,U> >
{
  typedef TypeTraitsUnknownTag NumericTag;
  typedef TypeTraitsScalarTag DimensionalityTag;

  VTKM_EXEC_CONT_EXPORT
  static vtkm::Pair<T,U> ZeroInitialization()
  {
    return vtkm::Pair<T,U>(TypeTraits<T>::ZeroInitialization(),
                           TypeTraits<U>::ZeroInitialization());
  }
};

} // namespace vtkm

#endif //vtk_m_TypeTraits_h
