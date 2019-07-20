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

#include <vtkm/internal/ListTagDetail.h>

#include <vtkm/StaticAssert.h>
#include <vtkm/internal/ExportMacros.h>

#include <type_traits>

namespace vtkm
{

namespace internal
{

template <typename ListTag>
struct ListTagCheck : std::is_base_of<vtkm::detail::ListRoot, ListTag>
{
  static constexpr bool Valid = std::is_base_of<vtkm::detail::ListRoot, ListTag>::value;
};

} // namespace internal

/// Checks that the argument is a proper list tag. This is a handy concept
/// check for functions and classes to make sure that a template argument is
/// actually a device adapter tag. (You can get weird errors elsewhere in the
/// code when a mistake is made.)
///
#define VTKM_IS_LIST_TAG(tag)                                                                      \
  VTKM_STATIC_ASSERT_MSG((::vtkm::internal::ListTagCheck<tag>::value),                             \
                         "Provided type is not a valid VTK-m list tag.")

namespace internal
{

namespace detail
{

template <typename ListTag>
struct ListTagAsBrigandListImpl
{
  VTKM_IS_LIST_TAG(ListTag);
  using type = typename ListTag::list;
};

} // namespace detail

/// Converts a ListTag to a brigand::list.
///
template <typename ListTag>
using ListTagAsBrigandList = typename detail::ListTagAsBrigandListImpl<ListTag>::type;

} // namespace internal


namespace detail
{

template <typename BrigandList, template <typename...> class Target>
struct ListTagApplyImpl;

template <typename... Ts, template <typename...> class Target>
struct ListTagApplyImpl<brigand::list<Ts...>, Target>
{
  using type = Target<Ts...>;
};

} // namespace detail

/// \brief Applies the list of types to a template.
///
/// Given a ListTag and a templated class, returns the class instantiated with the types
/// represented by the ListTag.
///
template <typename ListTag, template <typename...> class Target>
using ListTagApply =
  typename detail::ListTagApplyImpl<internal::ListTagAsBrigandList<ListTag>, Target>::type;

/// A special tag for a list that represents holding all potential values
///
/// Note: Can not be used with ForEach for obvious reasons.
struct ListTagUniversal : detail::ListRoot
{
  using list = vtkm::detail::ListBase<vtkm::detail::UniversalTag>;
};

/// A special tag for an empty list.
///
struct ListTagEmpty : detail::ListRoot
{
  using list = vtkm::detail::ListBase<>;
};

/// A tag that is a construction of two other tags joined together. This struct
/// can be subclassed and still behave like a list tag.
template <typename ListTag1, typename ListTag2>
struct ListTagJoin : detail::ListRoot
{
  VTKM_IS_LIST_TAG(ListTag1);
  VTKM_IS_LIST_TAG(ListTag2);
  using list = typename detail::ListJoin<internal::ListTagAsBrigandList<ListTag1>,
                                         internal::ListTagAsBrigandList<ListTag2>>::type;
};


/// A tag that is constructed by appending \c Type to \c ListTag.
template <typename ListTag, typename Type>
struct ListTagAppend : detail::ListRoot
{
  VTKM_IS_LIST_TAG(ListTag);
  using list = typename detail::ListJoin<internal::ListTagAsBrigandList<ListTag>,
                                         detail::ListBase<Type>>::type;
};

/// Append \c Type to \c ListTag only if \c ListTag does not already contain \c Type.
/// No checks are performed to see if \c ListTag itself has only unique elements.
template <typename ListTag, typename Type>
struct ListTagAppendUnique : detail::ListRoot
{
  VTKM_IS_LIST_TAG(ListTag);
  using list =
    typename detail::ListAppendUniqueImpl<internal::ListTagAsBrigandList<ListTag>, Type>::type;
};

/// A tag that consists of elements that are found in both tags. This struct
/// can be subclassed and still behave like a list tag.
template <typename ListTag1, typename ListTag2>
struct ListTagIntersect : detail::ListRoot
{
  VTKM_IS_LIST_TAG(ListTag1);
  VTKM_IS_LIST_TAG(ListTag2);
  using list = typename detail::ListIntersect<internal::ListTagAsBrigandList<ListTag1>,
                                              internal::ListTagAsBrigandList<ListTag2>>::type;
};

/// A list tag that consists of each item in another list tag fed into a template that takes
/// a single parameter.
template <typename ListTag, template <typename> class Transform>
struct ListTagTransform : detail::ListRoot
{
  VTKM_IS_LIST_TAG(ListTag);
  using list = brigand::transform<internal::ListTagAsBrigandList<ListTag>,
                                  brigand::bind<Transform, brigand::_1>>;
};

/// \brief Determines the number of types in the given list.
///
/// There is a static member named \c value that is set to the length of the list.
///
template <typename ListTag>
struct ListSize
{
  VTKM_IS_LIST_TAG(ListTag);
  static constexpr vtkm::IdComponent value =
    detail::ListSizeImpl<internal::ListTagAsBrigandList<ListTag>>::value;
};

/// For each typename represented by the list tag, call the functor with a
/// default instance of that type.
///
template <typename Functor, typename ListTag, typename... Args>
VTKM_CONT void ListForEach(Functor&& f, ListTag, Args&&... args)
{
  VTKM_IS_LIST_TAG(ListTag);
  detail::ListForEachImpl(std::forward<Functor>(f),
                          internal::ListTagAsBrigandList<ListTag>{},
                          std::forward<Args>(args)...);
}

/// Generate a tag that is the cross product of two other tags. The resulting
/// tag has the form of Tag< brigand::list<A1,B1>, brigand::list<A1,B2> .... >
///
template <typename ListTag1, typename ListTag2>
struct ListCrossProduct : detail::ListRoot
{
  VTKM_IS_LIST_TAG(ListTag1);
  VTKM_IS_LIST_TAG(ListTag2);
  using list =
    typename detail::ListCrossProductImpl<internal::ListTagAsBrigandList<ListTag1>,
                                          internal::ListTagAsBrigandList<ListTag2>>::type;
};

/// \brief Checks to see if the given \c Type is in the list pointed to by \c ListTag.
///
/// There is a static boolean named \c value that is set to true if the type is
/// contained in the list and false otherwise.
///
template <typename ListTag, typename Type>
struct ListContains
{
  VTKM_IS_LIST_TAG(ListTag);
  static constexpr bool value =
    detail::ListContainsImpl<Type, internal::ListTagAsBrigandList<ListTag>>::value;
};

/// \brief Finds the type at the given index.
///
/// This struct contains subtype \c type that resolves to the type at the given index.
///
template <typename ListTag, vtkm::IdComponent Index>
struct ListTypeAt
{
  VTKM_IS_LIST_TAG(ListTag);
  using type = brigand::at<internal::ListTagAsBrigandList<ListTag>,
                           std::integral_constant<vtkm::IdComponent, Index>>;
};

/// \brief Finds the index of the given type.
///
/// There is a static member named \c value that is set to the index of the given type. If the
/// given type is not in the list, the value is set to -1.
///
template <typename ListTag, typename Type>
struct ListIndexOf
{
  VTKM_IS_LIST_TAG(ListTag);
  static constexpr vtkm::IdComponent value =
    detail::ListIndexOfImpl<Type, internal::ListTagAsBrigandList<ListTag>, 0>::value;
};

} // namespace vtkm

#endif //vtk_m_ListTag_h
