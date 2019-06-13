//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_TypeTraits_h
#define vtk_m_TypeTraits_h

#include <vtkm/Types.h>

namespace vtkm
{

/// Tag used to identify types that aren't Real, Integer, Scalar or Vector.
///
struct TypeTraitsUnknownTag
{
};

/// Tag used to identify types that store real (floating-point) numbers. A
/// TypeTraits class will typedef this class to NumericTag if it stores real
/// numbers (or vectors of real numbers).
///
struct TypeTraitsRealTag
{
};

/// Tag used to identify types that store integer numbers. A TypeTraits class
/// will typedef this class to NumericTag if it stores integer numbers (or
/// vectors of integers).
///
struct TypeTraitsIntegerTag
{
};

/// Tag used to identify 0 dimensional types (scalars). Scalars can also be
/// treated like vectors when used with VecTraits. A TypeTraits class will
/// typedef this class to DimensionalityTag.
///
struct TypeTraitsScalarTag
{
};

/// Tag used to identify 1 dimensional types (vectors). A TypeTraits class will
/// typedef this class to DimensionalityTag.
///
struct TypeTraitsVectorTag
{
};

/// The TypeTraits class provides helpful compile-time information about the
/// basic types used in VTKm (and a few others for convenience). The majority
/// of TypeTraits contents are typedefs to tags that can be used to easily
/// override behavior of called functions.
///
template <typename T>
class TypeTraits
{
public:
  /// \brief A tag to determine whether the type is integer or real.
  ///
  /// This tag is either TypeTraitsRealTag or TypeTraitsIntegerTag.
  using NumericTag = vtkm::TypeTraitsUnknownTag;

  /// \brief A tag to determine whether the type has multiple components.
  ///
  /// This tag is either TypeTraitsScalarTag or TypeTraitsVectorTag. Scalars can
  /// also be treated as vectors.
  using DimensionalityTag = vtkm::TypeTraitsUnknownTag;

  VTKM_EXEC_CONT static T ZeroInitialization() { return T(); }
};

// Const types should have the same traits as their non-const counterparts.
//
template <typename T>
struct TypeTraits<const T> : TypeTraits<T>
{
};

#define VTKM_BASIC_REAL_TYPE(T)                                                                    \
  template <>                                                                                      \
  struct TypeTraits<T>                                                                             \
  {                                                                                                \
    using NumericTag = TypeTraitsRealTag;                                                          \
    using DimensionalityTag = TypeTraitsScalarTag;                                                 \
    VTKM_EXEC_CONT static T ZeroInitialization() { return T(); }                                   \
  };

#define VTKM_BASIC_INTEGER_TYPE(T)                                                                 \
  template <>                                                                                      \
  struct TypeTraits<T>                                                                             \
  {                                                                                                \
    using NumericTag = TypeTraitsIntegerTag;                                                       \
    using DimensionalityTag = TypeTraitsScalarTag;                                                 \
    VTKM_EXEC_CONT static T ZeroInitialization()                                                   \
    {                                                                                              \
      using ReturnType = T;                                                                        \
      return ReturnType();                                                                         \
    }                                                                                              \
  };

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
template <typename T, vtkm::IdComponent Size>
struct TypeTraits<vtkm::Vec<T, Size>>
{
  using NumericTag = typename vtkm::TypeTraits<T>::NumericTag;
  using DimensionalityTag = vtkm::TypeTraitsVectorTag;

  VTKM_EXEC_CONT
  static vtkm::Vec<T, Size> ZeroInitialization()
  {
    return vtkm::Vec<T, Size>(vtkm::TypeTraits<T>::ZeroInitialization());
  }
};

/// Traits for VecCConst types.
///
template <typename T>
struct TypeTraits<vtkm::VecCConst<T>>
{
  using NumericTag = typename vtkm::TypeTraits<T>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  VTKM_EXEC_CONT
  static vtkm::VecCConst<T> ZeroInitialization() { return vtkm::VecCConst<T>(); }
};

/// Traits for VecC types.
///
template <typename T>
struct TypeTraits<vtkm::VecC<T>>
{
  using NumericTag = typename vtkm::TypeTraits<T>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  VTKM_EXEC_CONT
  static vtkm::VecC<T> ZeroInitialization() { return vtkm::VecC<T>(); }
};

/// \brief Traits for Pair types.
///
template <typename T, typename U>
struct TypeTraits<vtkm::Pair<T, U>>
{
  using NumericTag = vtkm::TypeTraitsUnknownTag;
  using DimensionalityTag = vtkm::TypeTraitsScalarTag;

  VTKM_EXEC_CONT
  static vtkm::Pair<T, U> ZeroInitialization()
  {
    return vtkm::Pair<T, U>(TypeTraits<T>::ZeroInitialization(),
                            TypeTraits<U>::ZeroInitialization());
  }
};

} // namespace vtkm

#endif //vtk_m_TypeTraits_h
