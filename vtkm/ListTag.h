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
#ifndef vtk_m_ListTag_h
#define vtk_m_ListTag_h

#include <vtkm/internal/ListTagDetail.h>

#include <vtkm/StaticAssert.h>
#include <vtkm/internal/ExportMacros.h>

#include <type_traits>

namespace vtkm {

namespace internal {

template<typename ListTag>
struct ListTagCheck
{
  static VTKM_CONSTEXPR bool Valid = std::is_base_of<vtkm::detail::ListRoot,
                                                     ListTag>::value;
};

} // namespace internal

/// Checks that the argument is a proper list tag. This is a handy concept
/// check for functions and classes to make sure that a template argument is
/// actually a device adapter tag. (You can get weird errors elsewhere in the
/// code when a mistake is made.)
///
#define VTKM_IS_LIST_TAG(tag) \
  VTKM_STATIC_ASSERT_MSG( \
    (::vtkm::internal::ListTagCheck<tag>::Valid), \
    "Provided type is not a valid VTK-m list tag.")

namespace detail {

template<typename ListTag1, typename ListTag2>
struct ListJoin { };

} // namespace detail

/// A special tag for an empty list.
///
struct ListTagEmpty : detail::ListRoot {
  typedef detail::ListBase<void()> List;
};

/// A tag that is a construction of two other tags joined together. This struct
/// can be subclassed and still behave like a list tag.
template<typename ListTag1, typename ListTag2>
struct ListTagJoin : detail::ListRoot {
  typedef detail::ListJoin<ListTag1, ListTag2> List;
};

template<typename Functor, typename ListTag>
VTKM_CONT_EXPORT
void ListForEach(Functor &f, ListTag);

template<typename Functor, typename ListTag>
VTKM_CONT_EXPORT
void ListForEach(const Functor &f, ListTag);

namespace detail {

template<typename Functor, typename ListTag1, typename ListTag2>
VTKM_CONT_EXPORT
void ListForEachImpl(Functor &f, ListJoin<ListTag1, ListTag2>)
{
  vtkm::ListForEach(f, ListTag1());
  vtkm::ListForEach(f, ListTag2());
}

template<typename Functor, typename ListTag1, typename ListTag2>
VTKM_CONT_EXPORT
void ListForEachImpl(const Functor &f, ListJoin<ListTag1, ListTag2>)
{
  vtkm::ListForEach(f, ListTag1());
  vtkm::ListForEach(f, ListTag2());
}

} // namespace detail

/// For each typename represented by the list tag, call the functor with a
/// default instance of that type.
///
template<typename Functor, typename ListTag>
VTKM_CONT_EXPORT
void ListForEach(Functor &f, ListTag)
{
  VTKM_IS_LIST_TAG(ListTag);
  detail::ListForEachImpl(f, typename ListTag::List());
}

/// For each typename represented by the list tag, call the functor with a
/// default instance of that type.
///
template<typename Functor, typename ListTag>
VTKM_CONT_EXPORT
void ListForEach(const Functor &f, ListTag)
{
  VTKM_IS_LIST_TAG(ListTag);
  detail::ListForEachImpl(f, typename ListTag::List());
}

/// Checks to see if the given \c Type is in the list pointed to by \c ListTag.
/// There is a static boolean named \c value that is set to true if the type is
/// contained in the list and false otherwise.
///
template<typename ListTag, typename Type>
struct ListContains
{
  VTKM_IS_LIST_TAG(ListTag);
  static const bool value =
      detail::ListContainsImpl<typename ListTag::List,Type>::value;
};

namespace detail {

template<typename Type, typename ListTag1, typename ListTag2>
struct ListContainsImpl<ListJoin<ListTag1,ListTag2>, Type>
{
  static const bool value = (vtkm::ListContains<ListTag1,Type>::value ||
                             vtkm::ListContains<ListTag2,Type>::value);
};

} // namespace detail

} // namespace vtkm

#endif //vtk_m_ListTag_h
