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
#ifndef vtk_m_VecTraits_h
#define vtk_m_VecTraits_h

#include <vtkm/Types.h>

namespace vtkm
{

/// A tag for vectors that are "true" vectors (i.e. have more than one
/// component).
///
struct VecTraitsTagMultipleComponents
{
};

/// A tag for vectors that are really just scalars (i.e. have only one
/// component)
///
struct VecTraitsTagSingleComponent
{
};

/// A tag for vectors where the number of components are known at compile time.
///
struct VecTraitsTagSizeStatic
{
};

/// A tag for vectors where the number of components are not determined until
/// run time.
///
struct VecTraitsTagSizeVariable
{
};

namespace internal
{

template <vtkm::IdComponent numComponents>
struct VecTraitsMultipleComponentChooser
{
  using Type = vtkm::VecTraitsTagMultipleComponents;
};

template <>
struct VecTraitsMultipleComponentChooser<1>
{
  using Type = vtkm::VecTraitsTagSingleComponent;
};

} // namespace detail

/// The VecTraits class gives several static members that define how
/// to use a given type as a vector.
///
template <class VecType>
struct VecTraits
#ifdef VTKM_DOXYGEN_ONLY
{
  /// Type of the components in the vector.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// \brief Number of components in the vector.
  ///
  /// This is only defined for vectors of a static size.
  ///
  static constexpr vtkm::IdComponent NUM_COMPONENTS = VecType::NUM_COMPONENTS;

  /// Number of components in the given vector.
  ///
  static vtkm::IdComponent GetNumberOfComponents(const VecType& vec);

  /// \brief A tag specifying whether this vector has multiple components (i.e. is a "real" vector).
  ///
  /// This tag can be useful for creating specialized functions when a vector
  /// is really just a scalar.
  ///
  using HasMultipleComponents =
    typename internal::VecTraitsMultipleComponentChooser<NUM_COMPONENTS>::Type;

  /// \brief A tag specifying whether the size of this vector is known at compile time.
  ///
  /// If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set. If
  /// set to \c VecTraitsTagSizeVariable, then the number of components is not
  /// known at compile time and must be queried with \c GetNumberOfComponents.
  ///
  using IsSizeStatic = vtkm::VecTraitsTagSizeStatic;

  /// Returns the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT static const ComponentType& GetComponent(
    const typename std::remove_const<VecType>::type& vector,
    vtkm::IdComponent component);
  VTKM_EXEC_CONT static ComponentType& GetComponent(
    typename std::remove_const<VecType>::type& vector,
    vtkm::IdComponent component);

  /// Changes the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT static void SetComponent(VecType& vector,
                                          vtkm::IdComponent component,
                                          ComponentType value);

  /// Copies the components in the given vector into a given Vec object.
  ///
  template <vktm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(const VecType& src, vtkm::Vec<ComponentType, destSize>& dest);
};
#else  // VTKM_DOXYGEN_ONLY
  ;
#endif // VTKM_DOXYGEN_ONLY

// This partial specialization allows you to define a non-const version of
// VecTraits and have it still work for const version.
//
template <typename T>
struct VecTraits<const T> : VecTraits<T>
{
};

template <typename T, vtkm::IdComponent Size>
struct VecTraits<vtkm::Vec<T, Size>>
{
  using VecType = vtkm::Vec<T, Size>;

  /// Type of the components in the vector.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// Number of components in the vector.
  ///
  static constexpr vtkm::IdComponent NUM_COMPONENTS = VecType::NUM_COMPONENTS;

  /// Number of components in the given vector.
  ///
  VTKM_EXEC_CONT
  static vtkm::IdComponent GetNumberOfComponents(const VecType&) { return NUM_COMPONENTS; }

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  using HasMultipleComponents =
    typename internal::VecTraitsMultipleComponentChooser<NUM_COMPONENTS>::Type;

  /// A tag specifying whether the size of this vector is known at compile
  /// time. If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set.
  /// If set to \c VecTraitsTagSizeVariable, then the number of components is
  /// not known at compile time and must be queried with \c
  /// GetNumberOfComponents.
  ///
  using IsSizeStatic = vtkm::VecTraitsTagSizeStatic;

  /// Returns the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT
  static const ComponentType& GetComponent(const VecType& vector, vtkm::IdComponent component)
  {
    return vector[component];
  }
  VTKM_EXEC_CONT
  static ComponentType& GetComponent(VecType& vector, vtkm::IdComponent component)
  {
    return vector[component];
  }

  /// Changes the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT static void SetComponent(VecType& vector,
                                          vtkm::IdComponent component,
                                          ComponentType value)
  {
    vector[component] = value;
  }

  /// Converts whatever type this vector is into the standard VTKm Tuple.
  ///
  template <vtkm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(const VecType& src, vtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

template <typename T>
struct VecTraits<vtkm::VecC<T>>
{
  using VecType = vtkm::VecC<T>;

  /// Type of the components in the vector.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// Number of components in the given vector.
  ///
  VTKM_EXEC_CONT
  static vtkm::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  /// The size of a \c VecC is not known until runtime and can always
  /// potentially have multiple components, this is always set to \c
  /// HasMultipleComponents.
  ///
  using HasMultipleComponents = vtkm::VecTraitsTagMultipleComponents;

  /// A tag specifying whether the size of this vector is known at compile
  /// time. If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set.
  /// If set to \c VecTraitsTagSizeVariable, then the number of components is
  /// not known at compile time and must be queried with \c
  /// GetNumberOfComponents.
  ///
  using IsSizeStatic = vtkm::VecTraitsTagSizeVariable;

  /// Returns the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT
  static const ComponentType& GetComponent(const VecType& vector, vtkm::IdComponent component)
  {
    return vector[component];
  }
  VTKM_EXEC_CONT
  static ComponentType& GetComponent(VecType& vector, vtkm::IdComponent component)
  {
    return vector[component];
  }

  /// Changes the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT
  static void SetComponent(VecType& vector, vtkm::IdComponent component, ComponentType value)
  {
    vector[component] = value;
  }

  /// Converts whatever type this vector is into the standard VTKm Tuple.
  ///
  template <vtkm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(const VecType& src, vtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

template <typename T>
struct VecTraits<vtkm::VecCConst<T>>
{
  using VecType = vtkm::VecCConst<T>;

  /// Type of the components in the vector.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// Number of components in the given vector.
  ///
  VTKM_EXEC_CONT
  static vtkm::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  /// The size of a \c VecCConst is not known until runtime and can always
  /// potentially have multiple components, this is always set to \c
  /// HasMultipleComponents.
  ///
  using HasMultipleComponents = vtkm::VecTraitsTagMultipleComponents;

  /// A tag specifying whether the size of this vector is known at compile
  /// time. If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set.
  /// If set to \c VecTraitsTagSizeVariable, then the number of components is
  /// not known at compile time and must be queried with \c
  /// GetNumberOfComponents.
  ///
  using IsSizeStatic = vtkm::VecTraitsTagSizeVariable;

  /// Returns the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT
  static const ComponentType& GetComponent(const VecType& vector, vtkm::IdComponent component)
  {
    return vector[component];
  }

  /// Changes the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT
  static void SetComponent(VecType& vector, vtkm::IdComponent component, ComponentType value)
  {
    vector[component] = value;
  }

  /// Converts whatever type this vector is into the standard VTKm Tuple.
  ///
  template <vtkm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(const VecType& src, vtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

namespace internal
{
/// Used for overriding VecTraits for basic scalar types.
///
template <typename ScalarType>
struct VecTraitsBasic
{
  using ComponentType = ScalarType;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = 1;
  using HasMultipleComponents = vtkm::VecTraitsTagSingleComponent;
  using IsSizeStatic = vtkm::VecTraitsTagSizeStatic;

  VTKM_EXEC_CONT
  static vtkm::IdComponent GetNumberOfComponents(const ScalarType&) { return 1; }

  VTKM_EXEC_CONT
  static const ComponentType& GetComponent(const ScalarType& vector, vtkm::IdComponent)
  {
    return vector;
  }
  VTKM_EXEC_CONT
  static ComponentType& GetComponent(ScalarType& vector, vtkm::IdComponent) { return vector; }

  VTKM_EXEC_CONT static void SetComponent(ScalarType& vector,
                                          vtkm::IdComponent,
                                          ComponentType value)
  {
    vector = value;
  }

  template <vtkm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(const ScalarType& src, vtkm::Vec<ScalarType, destSize>& dest)
  {
    dest[0] = src;
  }
};
} // namespace internal

/// \brief VecTraits for Pair types
///
/// Although a pair would seem better as a size-2 vector, we treat it as a
/// scalar. This is because a \c Vec is assumed to have the same type for
/// every component, and a pair in general has a different type for each
/// component. Thus we treat a pair as a "scalar" unit.
///
template <typename T, typename U>
struct VecTraits<vtkm::Pair<T, U>> : public vtkm::internal::VecTraitsBasic<vtkm::Pair<T, U>>
{
};

} // anonymous namespace

#define VTKM_BASIC_TYPE_VECTOR(type)                                                               \
  namespace vtkm                                                                                   \
  {                                                                                                \
  template <>                                                                                      \
  struct VecTraits<type> : public vtkm::internal::VecTraitsBasic<type>                             \
  {                                                                                                \
  };                                                                                               \
  }

/// Allows you to treat basic types as if they were vectors.

VTKM_BASIC_TYPE_VECTOR(float)
VTKM_BASIC_TYPE_VECTOR(double)

VTKM_BASIC_TYPE_VECTOR(bool)
VTKM_BASIC_TYPE_VECTOR(char)
VTKM_BASIC_TYPE_VECTOR(signed char)
VTKM_BASIC_TYPE_VECTOR(unsigned char)
VTKM_BASIC_TYPE_VECTOR(short)
VTKM_BASIC_TYPE_VECTOR(unsigned short)
VTKM_BASIC_TYPE_VECTOR(int)
VTKM_BASIC_TYPE_VECTOR(unsigned int)
VTKM_BASIC_TYPE_VECTOR(long)
VTKM_BASIC_TYPE_VECTOR(unsigned long)
VTKM_BASIC_TYPE_VECTOR(long long)
VTKM_BASIC_TYPE_VECTOR(unsigned long long)

//#undef VTKM_BASIC_TYPE_VECTOR

#endif //vtk_m_VecTraits_h
