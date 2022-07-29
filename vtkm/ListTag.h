//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_ListTag_h
#define vtk_m_ListTag_h

#include <vtkm/Deprecated.h>

#include <vtkm/List.h>

#include <vtkm/StaticAssert.h>
#include <vtkm/internal/ExportMacros.h>

#include <type_traits>

struct VTKM_DEPRECATED(1.6, "ListTag.h is deprecated. Include List.h and use vtkm::List instead.")
  VTKmListTagHeaderDeprecationWarning
{
};

inline VTKmListTagHeaderDeprecationWarning IssueVTKmListTagHeaderDeprecationWarning()
{
  return {};
}

namespace vtkm
{
namespace detail
{

//-----------------------------------------------------------------------------

/// Base class that all ListTag classes inherit from. Helps identify lists
/// in macros like VTKM_IS_LIST_TAG.
///
struct ListRoot
{
};

template <class... T>
using ListBase = vtkm::List<T...>;

/// list value that is used to represent a list actually matches all values
struct UniversalTag
{
  //We never want this tag constructed, and by deleting the constructor
  //we get an error when trying to use this class with ForEach.
  UniversalTag() = delete;
};

} // namespace detail

//-----------------------------------------------------------------------------
/// A basic tag for a list of typenames. This struct can be subclassed
/// and still behave like a list tag.
/// @cond NONE
template <typename... ArgTypes>
struct VTKM_DEPRECATED(1.6, "ListTagBase replace by List. Note that List cannot be subclassed.")
  ListTagBase : detail::ListRoot
{
  using list = detail::ListBase<ArgTypes...>;
};
/// @endcond

/// A special tag for a list that represents holding all potential values
///
/// Note: Can not be used with ForEach for obvious reasons.
/// @cond NONE
struct VTKM_DEPRECATED(
  1.6,
  "ListTagUniversal replaced by ListUniversal. Note that ListUniversal cannot be subclassed.")
  ListTagUniversal : detail::ListRoot
{
  using list = vtkm::detail::ListBase<vtkm::detail::UniversalTag>;
};
/// @endcond

namespace internal
{

/// @cond NONE
template <typename ListTag>
struct ListTagCheck : std::is_base_of<vtkm::detail::ListRoot, ListTag>
{
  static constexpr bool Valid = std::is_base_of<vtkm::detail::ListRoot, ListTag>::value;
};
/// @endcond

} // namespace internal

namespace detail
{

/// @cond NONE
template <typename ListTag>
struct VTKM_DEPRECATED(1.6, "VTKM_IS_LIST_TAG replaced with VTKM_IS_LIST.") ListTagAssert
  : internal::IsList<ListTag>
{
};
/// @endcond

} // namespace detal

/// Checks that the argument is a proper list tag. This is a handy concept
/// check for functions and classes to make sure that a template argument is
/// actually a device adapter tag. (You can get weird errors elsewhere in the
/// code when a mistake is made.)
///
#define VTKM_IS_LIST_TAG(tag)                                         \
  VTKM_STATIC_ASSERT_MSG((::vtkm::detail::ListTagAssert<tag>::value), \
                         "Provided type is not a valid VTK-m list tag.")

namespace internal
{

namespace detail
{

/// @cond NONE
template <typename ListTag>
struct ListTagAsListImpl
{
  VTKM_DEPRECATED_SUPPRESS_BEGIN
  VTKM_IS_LIST_TAG(ListTag);
  using type = typename ListTag::list;
  VTKM_DEPRECATED_SUPPRESS_END
};
/// @endcond

} // namespace detail

/// Converts a ListTag to a vtkm::List.
///
template <typename ListTag>
using ListTagAsList = typename detail::ListTagAsListImpl<ListTag>::type;

VTKM_DEPRECATED_SUPPRESS_BEGIN
namespace detail
{

// Could use ListApply instead, but that causes deprecation warnings.
template <typename List>
struct ListAsListTagImpl;
template <typename... Ts>
struct ListAsListTagImpl<vtkm::List<Ts...>>
{
  using type = vtkm::ListTagBase<Ts...>;
};

} // namespace detail

template <typename List>
using ListAsListTag = typename detail::ListAsListTagImpl<List>::type;
VTKM_DEPRECATED_SUPPRESS_END

// This allows the new `List` operations work on `ListTag`s.
template <typename T>
struct AsListImpl
{
  VTKM_STATIC_ASSERT_MSG(ListTagCheck<T>::value,
                         "Attempted to use something that is not a List with a List operation.");
  VTKM_DEPRECATED_SUPPRESS_BEGIN
  using type = typename std::conditional<std::is_base_of<vtkm::ListTagUniversal, T>::value,
                                         vtkm::ListUniversal,
                                         ListTagAsList<T>>::type;
  VTKM_DEPRECATED_SUPPRESS_END
};

} // namespace internal


/// \brief Applies the list of types to a template.
///
/// Given a ListTag and a templated class, returns the class instantiated with the types
/// represented by the ListTag.
///
template <typename ListTag, template <typename...> class Target>
using ListTagApply VTKM_DEPRECATED(1.6, "ListTagApply replaced by ListApply.") =
  vtkm::ListApply<ListTag, Target>;

/// A special tag for an empty list.
///
/// @cond NONE
struct VTKM_DEPRECATED(
  1.6,
  "ListTagEmpty replaced by ListEmpty. Note that ListEmpty cannot be subclassed.") ListTagEmpty
  : detail::ListRoot
{
  using list = vtkm::detail::ListBase<>;
};
/// @endcond

/// A tag that is a construction of two other tags joined together. This struct
/// can be subclassed and still behave like a list tag.
/// @cond NONE
template <typename... ListTags>
struct VTKM_DEPRECATED(
  1.6,
  "ListTagJoin replaced by ListAppend. Note that ListAppend cannot be subclassed.") ListTagJoin
  : vtkm::internal::ListAsListTag<vtkm::ListAppend<ListTags...>>
{
};
/// @endcond


/// A tag that is constructed by appending \c Type to \c ListTag.
/// @cond NONE
template <typename ListTag, typename Type>
struct VTKM_DEPRECATED(1.6,
                       "ListTagAppend<List, Type> replaced by ListAppend<List, vtkm::List<Type>. "
                       "Note that ListAppend cannot be subclassed.") ListTagAppend
  : vtkm::internal::ListAsListTag<vtkm::ListAppend<ListTag, vtkm::List<Type>>>
{
};
/// @endcond

/// Append \c Type to \c ListTag only if \c ListTag does not already contain \c Type.
/// No checks are performed to see if \c ListTag itself has only unique elements.
/// @cond NONE
template <typename ListTag, typename Type>
struct VTKM_DEPRECATED(1.6) ListTagAppendUnique
  : std::conditional<
      vtkm::ListHas<ListTag, Type>::value,
      vtkm::internal::ListAsListTag<vtkm::internal::AsList<ListTag>>,
      vtkm::internal::ListAsListTag<vtkm::ListAppend<ListTag, vtkm::List<Type>>>>::type
{
};
/// @endcond

/// A tag that consists of elements that are found in both tags. This struct
/// can be subclassed and still behave like a list tag.
/// @cond NONE
template <typename ListTag1, typename ListTag2>
struct VTKM_DEPRECATED(
  1.6,
  "ListTagIntersect replaced by ListIntersect. Note that ListIntersect cannot be subclassed.")
  ListTagIntersect : vtkm::internal::ListAsListTag<vtkm::ListIntersect<ListTag1, ListTag2>>
{
};
/// @endcond

/// A list tag that consists of each item in another list tag fed into a template that takes
/// a single parameter.
/// @cond NONE
template <typename ListTag, template <typename> class Transform>
struct VTKM_DEPRECATED(
  1.6,
  "ListTagTransform replaced by ListTransform. Note that ListTransform cannot be subclassed.")
  ListTagTransform : vtkm::internal::ListAsListTag<vtkm::ListTransform<ListTag, Transform>>
{
};
/// @endcond

/// A list tag that takes an existing ListTag and a predicate template that is applied to
/// each type in the ListTag. Any type in the ListTag that has a value element equal to true
/// (the equivalent of std::true_type), that item will be removed from the list. For example
/// the following type
///
/// ```cpp
/// vtkm::ListTagRemoveIf<vtkm::ListTagBase<int, float, long long, double>, std::is_integral>
/// ```
///
/// resolves to a ListTag that is equivalent to `vtkm::ListTag<float, double>` because
/// `std::is_integral<int>` and `std::is_integral<long long>` resolve to `std::true_type`
/// whereas `std::is_integral<float>` and `std::is_integral<double>` resolve to
/// `std::false_type`.
/// @cond NONE
template <typename ListTag, template <typename> class Predicate>
struct VTKM_DEPRECATED(
  1.6,
  "ListTagRemoveIf replaced by ListRemoveIf. Note that ListRemoveIf cannot be subclassed.")
  ListTagRemoveIf : vtkm::internal::ListAsListTag<vtkm::ListRemoveIf<ListTag, Predicate>>
{
};
/// @endcond

/// Generate a tag that is the cross product of two other tags. The resulting
/// tag has the form of Tag< vtkm::List<A1,B1>, vtkm::List<A1,B2> .... >
///
/// Note that as of VTK-m 1.8, the behavior of this (already depreciated) operation
/// was changed to return pairs in vtkm::List instead of brigand::list.
///
/// @cond NONE
template <typename ListTag1, typename ListTag2>
struct VTKM_DEPRECATED(
  1.6,
  "ListCrossProduct replaced by ListCross. Note that ListCross cannot be subclassed.")
  ListCrossProduct : vtkm::internal::ListAsListTag<vtkm::ListCross<ListTag1, ListTag2>>
{
};
/// @endcond

/// \brief Checks to see if the given \c Type is in the list pointed to by \c ListTag.
///
/// There is a static boolean named \c value that is set to true if the type is
/// contained in the list and false otherwise.
///
/// @cond NONE
template <typename ListTag, typename Type>
struct VTKM_DEPRECATED(1.6, "ListContains replaced by ListHas.") ListContains
  : vtkm::ListHas<ListTag, Type>
{
};
/// @endcond

/// \brief Finds the type at the given index.
///
/// This struct contains subtype \c type that resolves to the type at the given index.
///
template <typename ListTag, vtkm::IdComponent Index>
struct VTKM_DEPRECATED(1.6, "ListTypeAt::type replaced by ListAt.") ListTypeAt
{
  VTKM_DEPRECATED_SUPPRESS_BEGIN
  VTKM_IS_LIST_TAG(ListTag);
  VTKM_DEPRECATED_SUPPRESS_END
  using type = vtkm::ListAt<ListTag, Index>;
};

} // namespace vtkm

#endif //vtk_m_ListTag_h
