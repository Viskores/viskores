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
#ifndef vtk_m_VecTraits_h
#define vtk_m_VecTraits_h

#include <vtkm/Types.h>

#include <boost/type_traits/remove_const.hpp>

namespace vtkm {

/// A tag for vectors that are "true" vectors (i.e. have more than one
/// component).
///
struct VecTraitsTagMultipleComponents { };

/// A tag for vectors that are really just scalars (i.e. have only one
/// component)
///
struct VecTraitsTagSingleComponent { };

namespace internal {

template<vtkm::IdComponent numComponents>
struct VecTraitsMultipleComponentChooser
{
  typedef VecTraitsTagMultipleComponents Type;
};

template<>
struct VecTraitsMultipleComponentChooser<1>
{
  typedef VecTraitsTagSingleComponent Type;
};

} // namespace detail

/// The VecTraits class gives several static members that define how
/// to use a given type as a vector.
///
template<class VecType>
struct VecTraits
#ifdef VTKM_DOXYGEN_ONLY
{
  /// Type of the components in the vector.
  ///
  typedef typename VecType::ComponentType ComponentType;

  /// Number of components in the vector.
  ///
  static const vtkm::IdComponent NUM_COMPONENTS = VecType::NUM_COMPONENTS;

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  typedef typename internal::VecTraitsMultipleComponentChooser<
      NUM_COMPONENTS>::Type HasMultipleComponents;

  /// Returns the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT_EXPORT static const ComponentType &GetComponent(
      const typename boost::remove_const<VecType>::type &vector,
      vtkm::IdComponent component);
  VTKM_EXEC_CONT_EXPORT static ComponentType &GetComponent(
      typename boost::remove_const<VecType>::type &vector,
      vtkm::IdComponent component);

  /// Changes the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT_EXPORT static void SetComponent(VecType &vector,
                                                vtkm::IdComponent component,
                                                ComponentType value);

  /// Converts whatever type this vector is into the standard VTK-m Vec.
  ///
  VTKM_EXEC_CONT_EXPORT
  static vtkm::Vec<ComponentType,NUM_COMPONENTS>
  ToVec(const VecType &vector);
};
#else // VTKM_DOXYGEN_ONLY
    ;
#endif // VTKM_DOXYGEN_ONLY

// This partial specialization allows you to define a non-const version of
// VecTraits and have it still work for const version.
//
template<typename T>
struct VecTraits<const T> : VecTraits<T>
{  };

template<typename T, vtkm::IdComponent Size>
struct VecTraits<vtkm::Vec<T,Size> >
{
  typedef vtkm::Vec<T,Size> VecType;

  /// Type of the components in the vector.
  ///
  typedef typename VecType::ComponentType ComponentType;

  /// Number of components in the vector.
  ///
  static const vtkm::IdComponent NUM_COMPONENTS = VecType::NUM_COMPONENTS;

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  typedef typename internal::VecTraitsMultipleComponentChooser<
      NUM_COMPONENTS>::Type HasMultipleComponents;

  /// Returns the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT_EXPORT
  static const ComponentType &GetComponent(const VecType &vector,
                                           vtkm::IdComponent component)
  {
    return vector[component];
  }
  VTKM_EXEC_CONT_EXPORT
  static ComponentType &GetComponent(VecType &vector, vtkm::IdComponent component) {
    return vector[component];
  }

  /// Changes the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT_EXPORT static void SetComponent(VecType &vector,
                                                vtkm::IdComponent component,
                                                ComponentType value) {
    vector[component] = value;
  }

  /// Converts whatever type this vector is into the standard VTKm Tuple.
  ///
  VTKM_EXEC_CONT_EXPORT
  static vtkm::Vec<ComponentType,NUM_COMPONENTS>
  ToVec(const VecType &vector)
  {
    return vector;
  }
};

namespace internal {
/// Used for overriding VecTraits for basic scalar types.
///
template<typename ScalarType>
struct VecTraitsBasic {
  typedef ScalarType ComponentType;
  static const vtkm::IdComponent NUM_COMPONENTS = 1;
  typedef VecTraitsTagSingleComponent HasMultipleComponents;

  VTKM_EXEC_CONT_EXPORT static const ComponentType &GetComponent(
      const ScalarType &vector,
      vtkm::IdComponent) {
    return vector;
  }
  VTKM_EXEC_CONT_EXPORT
  static ComponentType &GetComponent(ScalarType &vector, vtkm::IdComponent) {
    return vector;
  }

  VTKM_EXEC_CONT_EXPORT static void SetComponent(ScalarType &vector,
                                                vtkm::IdComponent,
                                                ComponentType value) {
    vector = value;
  }

  VTKM_EXEC_CONT_EXPORT
  static vtkm::Vec<ScalarType,1> ToVec(const ScalarType &vector)
  {
    return vtkm::Vec<ScalarType,1>(vector);
  }
};
} // namespace internal

} // anonymous namespace

#define VTKM_BASIC_TYPE_VECTOR(type) \
  namespace vtkm { \
    template<> \
    struct VecTraits<type> \
        : public vtkm::internal::VecTraitsBasic<type> { }; \
  }

/// Allows you to treat basic types as if they were vectors.

VTKM_BASIC_TYPE_VECTOR(vtkm::Float32)
VTKM_BASIC_TYPE_VECTOR(vtkm::Float64)
VTKM_BASIC_TYPE_VECTOR(vtkm::Int8)
VTKM_BASIC_TYPE_VECTOR(vtkm::UInt8)
VTKM_BASIC_TYPE_VECTOR(vtkm::Int16)
VTKM_BASIC_TYPE_VECTOR(vtkm::UInt16)
VTKM_BASIC_TYPE_VECTOR(vtkm::Int32)
VTKM_BASIC_TYPE_VECTOR(vtkm::UInt32)
VTKM_BASIC_TYPE_VECTOR(vtkm::Int64)
VTKM_BASIC_TYPE_VECTOR(vtkm::UInt64)

//#undef VTKM_BASIC_TYPE_VECTOR

#endif //vtk_m_VecTraits_h
