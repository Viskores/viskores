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

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/type_traits/remove_const.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {

/// A tag for vectors that are "true" vectors (i.e. have more than one
/// component).
///
struct VecTraitsTagMultipleComponents { };

/// A tag for vectors that are really just scalars (i.e. have only one
/// component)
///
struct VecTraitsTagSingleComponent { };

/// A tag for vectors where the number of components are known at compile time.
///
struct VecTraitsTagSizeStatic { };

/// A tag for vectors where the number of components are not determined until
/// run time.
///
struct VecTraitsTagSizeVariable { };

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

  /// \brief Number of components in the vector.
  ///
  /// This is only defined for vectors of a static size.
  ///
  static const vtkm::IdComponent NUM_COMPONENTS = VecType::NUM_COMPONENTS;

  /// Number of components in the given vector.
  ///
  static vtkm::IdComponent GetNumberOfComponents(const VecType &vec);

  /// \brief A tag specifying whether this vector has multiple components (i.e. is a "real" vector).
  ///
  /// This tag can be useful for creating specialized functions when a vector
  /// is really just a scalar.
  ///
  typedef typename internal::VecTraitsMultipleComponentChooser<
      NUM_COMPONENTS>::Type HasMultipleComponents;

  /// \brief A tag specifying whether the size of this vector is known at compile time.
  ///
  /// If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set. If
  /// set to \c VecTraitsTagSizeVariable, then the number of components is not
  /// known at compile time and must be queried with \c GetNumberOfComponents.
  ///
  typedef vtkm::VecTraitsTagSizeStatic IsSizeStatic;

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

  /// Copies the components in the given vector into a given Vec object.
  ///
  template<vktm::IdComponent destSize>
  VTKM_EXEC_CONT_EXPORT
  static void
  CopyInto(const VecType &src, vtkm::Vec<ComponentType,destSize> &dest);
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

  /// Number of components in the given vector.
  ///
  VTKM_EXEC_CONT_EXPORT
  static vtkm::IdComponent GetNumberOfComponents(const VecType &) {
    return NUM_COMPONENTS;
  }

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  typedef typename internal::VecTraitsMultipleComponentChooser<
      NUM_COMPONENTS>::Type HasMultipleComponents;

  /// A tag specifying whether the size of this vector is known at compile
  /// time. If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set.
  /// If set to \c VecTraitsTagSizeVariable, then the number of components is
  /// not known at compile time and must be queried with \c
  /// GetNumberOfComponents.
  ///
  typedef vtkm::VecTraitsTagSizeStatic IsSizeStatic;

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
  template<vtkm::IdComponent destSize>
  VTKM_EXEC_CONT_EXPORT
  static void
  CopyInto(const VecType &src, vtkm::Vec<ComponentType,destSize> &dest)
  {
    src.CopyInto(dest);
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
  typedef vtkm::VecTraitsTagSizeStatic IsSizeStatic;

  static vtkm::IdComponent GetNumberOfComponents(const ScalarType &) {
    return 1;
  }

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

  template<vtkm::IdComponent destSize>
  VTKM_EXEC_CONT_EXPORT
  static void CopyInto(const ScalarType &src,
                       vtkm::Vec<ScalarType,destSize> &dest)
  {
    dest[0] = src;
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
