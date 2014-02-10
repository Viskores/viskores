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
#ifndef vtkm_TypeTraits_h
#define vtkm_TypeTraits_h

#include <vtkm/Types.h>

namespace vtkm {

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
/// treated like vectors when used with VectorTraits. A TypeTraits class will
/// typedef this class to DimensionalityTag.
///
struct TypeTraitsScalarTag {};

/// Tag used to identify 1 dimensional types (vectors). A TypeTraits class will
/// typedef this class to DimensionalityTag.
///
struct TypeTraitsVectorTag {};

template<typename T> struct TypeTraits;

#ifdef VTKM_DOXYGEN_ONLY

/// The TypeTraits class provides helpful compile-time information about the
/// basic types used in VTKm (and a few others for convienience). The majority
/// of TypeTraits contents are typedefs to tags that can be used to easily
/// override behavior of called functions.
///
template<typename T>
class TypeTraits {
  typedef int tag_type; // Shut up, test compile.
public:

  /// \brief A tag to determing whether the type is integer or real.
  ///
  /// This tag is either TypeTraitsRealTag or TypeTraitsIntegerTag.
  typedef tag_type NumericTag;

  /// \brief A tag to determine whether the type has multiple components.
  ///
  /// This tag is either TypeTraitsScalarTag or TypeTraitsVectorTag. Scalars can
  /// also be treated as vectors.
  typedef tag_type DimensionalityTag;
};

#endif //VTKM_DOXYGEN_ONLY

// Const types should have the same traits as their non-const counterparts.
//
template<typename T>
struct TypeTraits<const T> : TypeTraits<T>
{  };

#define VTKM_BASIC_REAL_TYPE(T) \
template<> struct TypeTraits<T> { \
  typedef TypeTraitsRealTag NumericTag; \
  typedef TypeTraitsScalarTag DimensionalityTag; \
}

#define VTKM_BASIC_INTEGER_TYPE(T) \
template<> struct TypeTraits<T> { \
  typedef TypeTraitsIntegerTag NumericTag; \
  typedef TypeTraitsScalarTag DimensionalityTag; \
}

/// Traits for basic C++ types.
///

VTKM_BASIC_REAL_TYPE(float);
VTKM_BASIC_REAL_TYPE(double);
VTKM_BASIC_INTEGER_TYPE(char);
VTKM_BASIC_INTEGER_TYPE(unsigned char);
VTKM_BASIC_INTEGER_TYPE(short);
VTKM_BASIC_INTEGER_TYPE(unsigned short);
VTKM_BASIC_INTEGER_TYPE(int);
VTKM_BASIC_INTEGER_TYPE(unsigned int);
#if VTKM_SIZE_LONG == 8
VTKM_BASIC_INTEGER_TYPE(long);
VTKM_BASIC_INTEGER_TYPE(unsigned long);
#elif VTKM_SIZE_LONG_LONG == 8
VTKM_BASIC_INTEGER_TYPE(long long);
VTKM_BASIC_INTEGER_TYPE(unsigned long long);
#else
#error No implementation for 64-bit integer traits.
#endif

#undef VTKM_BASIC_REAL_TYPE
#undef VTKM_BASIC_INTEGER_TYPE

#define VTKM_VECTOR_TYPE(T, NTag) \
template<> struct TypeTraits<T> { \
  typedef NTag NumericTag; \
  typedef TypeTraitsVectorTag DimensionalityTag; \
}

/// Traits for vector types.
///

VTKM_VECTOR_TYPE(vtkm::Id3, TypeTraitsIntegerTag);
VTKM_VECTOR_TYPE(vtkm::Vector2, TypeTraitsRealTag);
VTKM_VECTOR_TYPE(vtkm::Vector3, TypeTraitsRealTag);
VTKM_VECTOR_TYPE(vtkm::Vector4, TypeTraitsRealTag);

#undef VTKM_VECTOR_TYPE

/// Traits for tuples.
///
template<typename T, int Size> struct TypeTraits<vtkm::Tuple<T, Size> > {
  typedef typename TypeTraits<T>::NumericTag NumericTag;
  typedef TypeTraitsVectorTag DimensionalityTag;
};

} // namespace vtkm

#endif //vtkm_TypeTraits_h
