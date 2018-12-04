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
  using list = typename detail::ListJoin<typename ListTag1::list, typename ListTag2::list>::type;
};


/// A tag that is constructed by appending \c Type to \c ListTag.
template <typename ListTag, typename Type>
struct ListTagAppend : detail::ListRoot
{
  using list = typename detail::ListJoin<typename ListTag::list, detail::ListBase<Type>>::type;
};

/// Append \c Type to \c ListTag only if \c ListTag does not already contain \c Type.
/// No checks are performed to see if \c ListTag itself has only unique elements.
template <typename ListTag, typename Type>
struct ListTagAppendUnique : detail::ListRoot
{
  using list = typename detail::ListAppendUniqueImpl<typename ListTag::list, Type>::type;
};

/// A tag that consists of elements that are found in both tags. This struct
/// can be subclassed and still behave like a list tag.
template <typename ListTag1, typename ListTag2>
struct ListTagIntersect : detail::ListRoot
{
  using list =
    typename detail::ListIntersect<typename ListTag1::list, typename ListTag2::list>::type;
};

/// For each typename represented by the list tag, call the functor with a
/// default instance of that type.
///
template <typename Functor, typename ListTag, typename... Args>
VTKM_CONT void ListForEach(Functor&& f, ListTag, Args&&... args)
{
  VTKM_IS_LIST_TAG(ListTag);
  detail::ListForEachImpl(
    std::forward<Functor>(f), typename ListTag::list{}, std::forward<Args>(args)...);
}

/// Generate a tag that is the cross product of two other tags. The resulting
// a tag has the form of Tag< brigand::list<A1,B1>, brigand::list<A1,B2> .... >
///
template <typename ListTag1, typename ListTag2>
struct ListCrossProduct : detail::ListRoot
{
  using list =
    typename detail::ListCrossProductImpl<typename ListTag1::list, typename ListTag2::list>::type;
};

/// Checks to see if the given \c Type is in the list pointed to by \c ListTag.
/// There is a static boolean named \c value that is set to true if the type is
/// contained in the list and false otherwise.
///
template <typename ListTag, typename Type>
struct ListContains
{
  VTKM_IS_LIST_TAG(ListTag);
  static constexpr bool value = detail::ListContainsImpl<Type, typename ListTag::list>::value;
};

} // namespace vtkm

#endif //vtk_m_ListTag_h
