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
#ifndef vtk_m_VectorTraits_h
#define vtk_m_VectorTraits_h

#include <vtkm/Types.h>

#include <boost/type_traits/remove_const.hpp>

namespace vtkm {

/// A tag for vectors that are "true" vectors (i.e. have more than one
/// component).
///
struct VectorTraitsTagMultipleComponents { };

/// A tag for vectors that a really just scalars (i.e. have only one component)
///
struct VectorTraitsTagSingleComponent { };

namespace internal {

template<int numComponents>
struct VectorTraitsMultipleComponentChooser
{
  typedef VectorTraitsTagMultipleComponents Type;
};

template<>
struct VectorTraitsMultipleComponentChooser<1>
{
  typedef VectorTraitsTagSingleComponent Type;
};

} // namespace detail

/// The VectorTraits class gives several static members that define how
/// to use a given type as a vector.
///
template<class VectorType>
struct VectorTraits
#ifdef VTKM_DOXYGEN_ONLY
{
  /// Type of the components in the vector.
  ///
  typedef typename VectorType::ComponentType ComponentType;

  /// Number of components in the vector.
  ///
  static const int NUM_COMPONENTS = VectorType::NUM_COMPONENTS;

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  typedef typename internal::VectorTraitsMultipleComponentChooser<
      NUM_COMPONENTS>::Type HasMultipleComponents;

  /// Returns the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT_EXPORT static const ComponentType &GetComponent(
      const typename boost::remove_const<VectorType>::type &vector,
      int component);
  VTKM_EXEC_CONT_EXPORT static ComponentType &GetComponent(
      typename boost::remove_const<VectorType>::type &vector,
      int component);

  /// Changes the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT_EXPORT static void SetComponent(VectorType &vector,
                                                int component,
                                                ComponentType value);

  /// Converts whatever type this vector is into the standard VTKm Tuple.
  ///
  VTKM_EXEC_CONT_EXPORT
  static vtkm::Tuple<ComponentType,NUM_COMPONENTS>
  ToTuple(const VectorType &vector);
};
#else // VTKM_DOXYGEN_ONLY
    ;
#endif // VTKM_DOXYGEN_ONLY

// This partial specialization allows you to define a non-const version of
// VectorTraits and have it still work for const version.
//
template<typename T>
struct VectorTraits<const T> : VectorTraits<T>
{  };

template<typename T, int Size>
struct VectorTraits<vtkm::Tuple<T,Size> >
{
  typedef vtkm::Tuple<T,Size> VectorType;

  /// Type of the components in the vector.
  ///
  typedef typename VectorType::ComponentType ComponentType;

  /// Number of components in the vector.
  ///
  static const int NUM_COMPONENTS = VectorType::NUM_COMPONENTS;

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  typedef typename internal::VectorTraitsMultipleComponentChooser<
      NUM_COMPONENTS>::Type HasMultipleComponents;

  /// Returns the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT_EXPORT
  static const ComponentType &GetComponent(const VectorType &vector,
                                           int component)
  {
    return vector[component];
  }
  VTKM_EXEC_CONT_EXPORT
  static ComponentType &GetComponent(VectorType &vector, int component) {
    return vector[component];
  }

  /// Changes the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT_EXPORT static void SetComponent(VectorType &vector,
                                                int component,
                                                ComponentType value) {
    vector[component] = value;
  }

  /// Converts whatever type this vector is into the standard VTKm Tuple.
  ///
  VTKM_EXEC_CONT_EXPORT
  static vtkm::Tuple<ComponentType,NUM_COMPONENTS>
  ToTuple(const VectorType &vector)
  {
    return vector;
  }
};

namespace internal {
/// Used for overriding VectorTraits for basic scalar types.
///
template<typename ScalarType>
struct VectorTraitsBasic {
  typedef ScalarType ComponentType;
  static const int NUM_COMPONENTS = 1;
  typedef VectorTraitsTagSingleComponent HasMultipleComponents;

  VTKM_EXEC_CONT_EXPORT static const ComponentType &GetComponent(
      const ScalarType &vector,
      int) {
    return vector;
  }
  VTKM_EXEC_CONT_EXPORT
  static ComponentType &GetComponent(ScalarType &vector, int) {
    return vector;
  }

  VTKM_EXEC_CONT_EXPORT static void SetComponent(ScalarType &vector,
                                                int,
                                                ComponentType value) {
    vector = value;
  }

  VTKM_EXEC_CONT_EXPORT
  static vtkm::Tuple<ScalarType,1> ToTuple(const ScalarType &vector)
  {
    return vtkm::Tuple<ScalarType,1>(vector);
  }
};
}

#define VTKM_BASIC_TYPE_VECTOR(type) \
  template<> \
  struct VectorTraits<type> \
      : public vtkm::internal::VectorTraitsBasic<type> { };/* \
  template<> \
  struct VectorTraits<const type> \
      : public vtkm::internal::VectorTraitsBasic<type> { }*/

/// Allows you to treat basic types as if they were vectors.

VTKM_BASIC_TYPE_VECTOR(float);
VTKM_BASIC_TYPE_VECTOR(double);
VTKM_BASIC_TYPE_VECTOR(char);
VTKM_BASIC_TYPE_VECTOR(unsigned char);
VTKM_BASIC_TYPE_VECTOR(short);
VTKM_BASIC_TYPE_VECTOR(unsigned short);
VTKM_BASIC_TYPE_VECTOR(int);
VTKM_BASIC_TYPE_VECTOR(unsigned int);
#if VTKM_SIZE_LONG == 8
VTKM_BASIC_TYPE_VECTOR(long);
VTKM_BASIC_TYPE_VECTOR(unsigned long);
#elif VTKM_SIZE_LONG_LONG == 8
VTKM_BASIC_TYPE_VECTOR(long long);
VTKM_BASIC_TYPE_VECTOR(unsigned long long);
#else
#error No implementation for 64-bit vector traits.
#endif

#undef VTKM_BASIC_TYPE_VECTOR

}

#endif //vtk_m_VectorTraits_h
