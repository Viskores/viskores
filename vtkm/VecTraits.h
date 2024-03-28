//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_VecTraits_h
#define vtk_m_VecTraits_h

#include <vtkm/Deprecated.h>
#include <vtkm/StaticAssert.h>
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

/// \brief Traits that can be queried to treat any type as a `Vec`.
///
/// The VecTraits class gives several static members that define how
/// to use a given type as a vector. This is useful for templated
/// functions and methods that have a parameter that could be either
/// a standard scalar type or a `Vec` or some other `Vec`-like
/// object. When using this class, scalar objects are treated like
/// a `Vec` of size 1.
///
/// The default implementation of this template treats the type as
/// a scalar. Types that actually behave like vectors should
/// specialize this template to provide the proper information.
///
template <class T>
struct VTKM_NEVER_EXPORT VecTraits
{
  // The base VecTraits should not be used with qualifiers.
  VTKM_STATIC_ASSERT_MSG((std::is_same<std::remove_pointer_t<std::decay_t<T>>, T>::value),
                         "The base VecTraits should not be used with qualifiers.");

  /// \brief Type of the components in the vector.
  ///
  /// If the type is really a scalar, then the component type is the same as the scalar type.
  ///
  using ComponentType = T;

  /// \brief Base component type in the vector.
  ///
  /// Similar to ComponentType except that for nested vectors (e.g. Vec<Vec<T, M>, N>), it
  /// returns the base scalar type at the end of the composition (T in this example).
  ///
  using BaseComponentType = T;

  /// \brief Number of components in the vector.
  ///
  /// This is only defined for vectors of a static size. That is, `NUM_COMPONENTS`
  /// is not available when `IsSizeStatic` is set to `vtkm::VecTraitsTagSizeVariable`.
  ///
  static constexpr vtkm::IdComponent NUM_COMPONENTS = 1;

  /// @brief Returns the number of components in the given vector.
  ///
  /// The result of `GetNumberOfComponents()` is the same value of `NUM_COMPONENTS`
  /// for vector types that have a static size (that is, `IsSizeStatic` is
  /// `vtkm::VecTraitsTagSizeStatic`). But unlike `NUM_COMPONENTS`, `GetNumberOfComponents()`
  /// works for vectors of any type.
  ///
  static constexpr vtkm::IdComponent GetNumberOfComponents(const T&) { return NUM_COMPONENTS; }

  /// \brief A tag specifying whether this vector has multiple components (i.e. is a "real" vector).
  ///
  /// This type is set to either `vtkm::VecTraitsTagSingleComponent` if the vector length
  /// is size 1 or `vtkm::VecTraitsTagMultipleComponents` otherwise.
  /// This tag can be useful for creating specialized functions when a vector is really
  /// just a scalar. If the vector type is of variable size (that is, `IsSizeStatic` is
  /// `vtkm::VecTraitsTagSizeVariable`), then `HasMultipleComponents` might be
  /// `vtkm::VecTraitsTagMultipleComponents` even when at run time there is only one component.
  ///
  using HasMultipleComponents = vtkm::VecTraitsTagSingleComponent;

  /// \brief A tag specifying whether the size of this vector is known at compile time.
  ///
  /// If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set. If
  /// set to \c VecTraitsTagSizeVariable, then the number of components is not
  /// known at compile time and must be queried with \c GetNumberOfComponents.
  ///
  using IsSizeStatic = vtkm::VecTraitsTagSizeStatic;

  /// Returns the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT static const ComponentType& GetComponent(const T& vector,
                                                          vtkm::IdComponent vtkmNotUsed(component))
  {
    return vector;
  }
  /// @copydoc GetComponent
  VTKM_EXEC_CONT static ComponentType& GetComponent(T& vector,
                                                    vtkm::IdComponent vtkmNotUsed(component))
  {
    return vector;
  }

  /// Changes the value in a given component of the vector.
  ///
  VTKM_EXEC_CONT static void SetComponent(T& vector,
                                          vtkm::IdComponent vtkmNotUsed(component),
                                          ComponentType value)
  {
    vector = value;
  }

  /// \brief Get a vector of the same type but with a different component.
  ///
  /// This type resolves to another vector with a different component type. For example,
  /// `vtkm::VecTraits<vtkm::Vec<T, N>>::%ReplaceComponentType<T2>` is `vtkm::Vec<T2, N>`. This
  /// replacement is not recursive. So `VecTraits<Vec<Vec<T, M>, N>::%ReplaceComponentType<T2>` is
  /// `vtkm::Vec<T2, N>`.
  ///
  // Note: the `%` in the code samples above is a hint to doxygen to avoid attempting
  // to link to the object (i.e. `ReplaceBaseComponentType`), which results in a warning.
  // The `%` is removed from the doxygen text.
  template <typename NewComponentType>
  using ReplaceComponentType = NewComponentType;

  /// \brief Get a vector of the same type but with a different base component.
  ///
  /// This type resolves to another vector with a different base component type. The replacement
  /// is recursive for nested types. For example,
  /// `VecTraits<Vec<Vec<T, M>, N>::%ReplaceBaseComponentType<T2>` is `Vec<Vec<T2, M>, N>`.
  ///
  // Note: the `%` in the code samples above is a hint to doxygen to avoid attempting
  // to link to the object (i.e. `ReplaceBaseComponentType`), which results in a warning.
  // The `%` is removed from the doxygen text.
  template <typename NewComponentType>
  using ReplaceBaseComponentType = NewComponentType;

  /// Copies the components in the given vector into a given Vec object.
  ///
  template <vtkm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(const T& src, vtkm::Vec<ComponentType, destSize>& dest)
  {
    dest[0] = src;
  }
};

template <typename T>
using HasVecTraits VTKM_DEPRECATED(2.1, "All types now have VecTraits defined.") = std::true_type;

// These partial specializations allow VecTraits to work with const and reference qualifiers.
template <typename T>
struct VTKM_NEVER_EXPORT VecTraits<const T> : VecTraits<T>
{
};
template <typename T>
struct VTKM_NEVER_EXPORT VecTraits<T&> : VecTraits<T>
{
};
template <typename T>
struct VTKM_NEVER_EXPORT VecTraits<const T&> : VecTraits<T>
{
};

// This partial specialization allows VecTraits to work with pointers.
template <typename T>
struct VTKM_NEVER_EXPORT VecTraits<T*> : VecTraits<T>
{
  VTKM_EXEC_CONT static vtkm::IdComponent GetNumberOfComponents(const T* vector)
  {
    return VecTraits<T>::GetNumberOfComponents(*vector);
  }
  VTKM_EXEC_CONT static auto GetComponent(const T* vector, vtkm::IdComponent component)
    -> decltype(VecTraits<T>::GetComponent(*vector, component))
  {
    return VecTraits<T>::GetComponent(*vector, component);
  }
  VTKM_EXEC_CONT static auto GetComponent(T* vector, vtkm::IdComponent component)
    -> decltype(VecTraits<T>::GetComponent(*vector, component))
  {
    return VecTraits<T>::GetComponent(*vector, component);
  }
  VTKM_EXEC_CONT static void SetComponent(T* vector,
                                          vtkm::IdComponent component,
                                          typename VecTraits<T>::ComponentType value)
  {
    VecTraits<T>::SetComponent(*vector, component, value);
  }
  template <typename NewComponentType>
  using ReplaceComponentType =
    typename VecTraits<T>::template ReplaceComponentType<NewComponentType>*;
  template <typename NewComponentType>
  using ReplaceBaseComponentType =
    typename VecTraits<T>::template ReplaceBaseComponentType<NewComponentType>*;
  template <vtkm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(
    const T* src,
    vtkm::Vec<typename VecTraits<T>::ComponentType, destSize>& dest)
  {
    VecTraits<T>::CopyInto(*src, dest);
  }
};
template <typename T>
struct VTKM_NEVER_EXPORT VecTraits<const T*> : VecTraits<T*>
{
};

#if defined(VTKM_GCC) && (__GNUC__ <= 5)
namespace detail
{

template <typename NewT, vtkm::IdComponent Size>
struct VecReplaceComponentTypeGCC4or5
{
  using type = vtkm::Vec<NewT, Size>;
};

template <typename T, vtkm::IdComponent Size, typename NewT>
struct VecReplaceBaseComponentTypeGCC4or5
{
  using type =
    vtkm::Vec<typename vtkm::VecTraits<T>::template ReplaceBaseComponentType<NewT>, Size>;
};

} // namespace detail
#endif // GCC Version 4.8

namespace internal
{

template <vtkm::IdComponent numComponents, typename ComponentType>
struct VecTraitsMultipleComponentChooser
{
  using Type = vtkm::VecTraitsTagMultipleComponents;
};

template <typename ComponentType>
struct VecTraitsMultipleComponentChooser<1, ComponentType>
{
  using Type = typename vtkm::VecTraits<ComponentType>::HasMultipleComponents;
};

} // namespace internal

template <typename T, vtkm::IdComponent Size>
struct VTKM_NEVER_EXPORT VecTraits<vtkm::Vec<T, Size>>
{
  using VecType = vtkm::Vec<T, Size>;

  /// \brief Type of the components in the vector.
  ///
  /// If the type is really a scalar, then the component type is the same as the scalar type.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// \brief Base component type in the vector.
  ///
  /// Similar to ComponentType except that for nested vectors (e.g. Vec<Vec<T, M>, N>), it
  /// returns the base scalar type at the end of the composition (T in this example).
  ///
  using BaseComponentType = typename vtkm::VecTraits<ComponentType>::BaseComponentType;

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
    typename internal::VecTraitsMultipleComponentChooser<NUM_COMPONENTS, ComponentType>::Type;

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

/// \brief Get a vector of the same type but with a different component.
///
/// This type resolves to another vector with a different component type. For example,
/// @code vtkm::VecTraits<vtkm::Vec<T, N>>::ReplaceComponentType<T2> @endcode is vtkm::Vec<T2, N>.
/// This replacement is not recursive. So @code VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> @endcode
/// is vtkm::Vec<T2, N>.
///@{
#if defined(VTKM_GCC) && (__GNUC__ <= 5)
  // Silly workaround for bug in GCC <= 5
  template <typename NewComponentType>
  using ReplaceComponentType =
    typename detail::VecReplaceComponentTypeGCC4or5<NewComponentType, Size>::type;
#else // !GCC <= 5
  template <typename NewComponentType>
  using ReplaceComponentType = vtkm::Vec<NewComponentType, Size>;
#endif
///@}

/// \brief Get a vector of the same type but with a different base component.
///
/// This type resolves to another vector with a different base component type. The replacement
/// is recursive for nested types. For example,
/// @code VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> @endcode is Vec<Vec<T2, M>, N>.
///@{
#if defined(VTKM_GCC) && (__GNUC__ <= 5)
  // Silly workaround for bug in GCC <= 5
  template <typename NewComponentType>
  using ReplaceBaseComponentType =
    typename detail::VecReplaceBaseComponentTypeGCC4or5<T, Size, NewComponentType>::type;
#else // !GCC <= 5
  template <typename NewComponentType>
  using ReplaceBaseComponentType = vtkm::Vec<
    typename vtkm::VecTraits<ComponentType>::template ReplaceBaseComponentType<NewComponentType>,
    Size>;
#endif
  ///@}

  /// Converts whatever type this vector is into the standard VTKm Tuple.
  ///
  template <vtkm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(const VecType& src, vtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

template <typename T>
struct VTKM_NEVER_EXPORT VecTraits<vtkm::VecC<T>>
{
  using VecType = vtkm::VecC<T>;

  /// \brief Type of the components in the vector.
  ///
  /// If the type is really a scalar, then the component type is the same as the scalar type.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// \brief Base component type in the vector.
  ///
  /// Similar to ComponentType except that for nested vectors (e.g. Vec<Vec<T, M>, N>), it
  /// returns the base scalar type at the end of the composition (T in this example).
  ///
  using BaseComponentType = typename vtkm::VecTraits<ComponentType>::BaseComponentType;

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

  /// \brief Get a vector of the same type but with a different component.
  ///
  /// This type resolves to another vector with a different component type. For example,
  /// @code vtkm::VecTraits<vtkm::Vec<T, N>>::ReplaceComponentType<T2> @endcode is vtkm::Vec<T2, N>.
  /// This replacement is not recursive. So @code VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> @endcode
  /// is vtkm::Vec<T2, N>.
  ///
  template <typename NewComponentType>
  using ReplaceComponentType = vtkm::VecC<NewComponentType>;

  /// \brief Get a vector of the same type but with a different base component.
  ///
  /// This type resolves to another vector with a different base component type. The replacement
  /// is recursive for nested types. For example,
  /// @code VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> @endcode is Vec<Vec<T2, M>, N>.
  ///
  template <typename NewComponentType>
  using ReplaceBaseComponentType = vtkm::VecC<
    typename vtkm::VecTraits<ComponentType>::template ReplaceBaseComponentType<NewComponentType>>;

  /// Converts whatever type this vector is into the standard VTKm Tuple.
  ///
  template <vtkm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(const VecType& src, vtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

template <typename T>
struct VTKM_NEVER_EXPORT VecTraits<vtkm::VecCConst<T>>
{
  using VecType = vtkm::VecCConst<T>;

  /// \brief Type of the components in the vector.
  ///
  /// If the type is really a scalar, then the component type is the same as the scalar type.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// \brief Base component type in the vector.
  ///
  /// Similar to ComponentType except that for nested vectors (e.g. Vec<Vec<T, M>, N>), it
  /// returns the base scalar type at the end of the composition (T in this example).
  ///
  using BaseComponentType = typename vtkm::VecTraits<ComponentType>::BaseComponentType;

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

  /// \brief Get a vector of the same type but with a different component.
  ///
  /// This type resolves to another vector with a different component type. For example,
  /// @code vtkm::VecTraits<vtkm::Vec<T, N>>::ReplaceComponentType<T2> @endcode is vtkm::Vec<T2, N>.
  /// This replacement is not recursive. So @code VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> @endcode
  /// is vtkm::Vec<T2, N>.
  ///
  template <typename NewComponentType>
  using ReplaceComponentType = vtkm::VecCConst<NewComponentType>;

  /// \brief Get a vector of the same type but with a different base component.
  ///
  /// This type resolves to another vector with a different base component type. The replacement
  /// is recursive for nested types. For example,
  /// @code VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> @endcode is Vec<Vec<T2, M>, N>.
  ///
  template <typename NewComponentType>
  using ReplaceBaseComponentType = vtkm::VecCConst<
    typename vtkm::VecTraits<ComponentType>::template ReplaceBaseComponentType<NewComponentType>>;

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
struct VTKM_DEPRECATED(2.1, "VecTraitsBasic is now the default implementation for VecTraits.")
  VTKM_NEVER_EXPORT VecTraitsBasic
{
  using ComponentType = ScalarType;
  using BaseComponentType = ScalarType;
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

  template <typename NewComponentType>
  using ReplaceComponentType = NewComponentType;

  template <typename NewComponentType>
  using ReplaceBaseComponentType = NewComponentType;

  template <vtkm::IdComponent destSize>
  VTKM_EXEC_CONT static void CopyInto(const ScalarType& src, vtkm::Vec<ScalarType, destSize>& dest)
  {
    dest[0] = src;
  }
};

template <typename T>
struct VTKM_DEPRECATED(2.1 "VecTraits now safe to use on any type.") VTKM_NEVER_EXPORT SafeVecTraits
  : vtkm::VecTraits<T>
{
};

} // namespace internal

namespace detail
{

struct VTKM_DEPRECATED(2.1,
                       "VTKM_BASIC_TYPE_VECTOR is no longer necessary because VecTraits implements "
                       "basic type by default.") VTKM_BASIC_TYPE_VECTOR_is_deprecated
{
};

template <typename T>
struct issue_VTKM_BASIC_TYPE_VECTOR_deprecation_warning;

}

} // namespace vtkm

#define VTKM_BASIC_TYPE_VECTOR(type)                            \
  namespace vtkm                                                \
  {                                                             \
  namespace detail                                              \
  {                                                             \
  template <>                                                   \
  struct issue_VTKM_BASIC_TYPE_VECTOR_deprecation_warning<type> \
    : public vtkm::detail::VTKM_BASIC_TYPE_VECTOR_is_deprecated \
  {                                                             \
  };                                                            \
  }                                                             \
  }

#endif //vtk_m_VecTraits_h
