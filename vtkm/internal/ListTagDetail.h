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

#ifndef vtk_m_internal_ListTagDetail_h
#define vtk_m_internal_ListTagDetail_h

#if !defined(vtk_m_ListTag_h) && !defined(VTKM_TEST_HEADER_BUILD)
#error ListTagDetail.h must be included from ListTag.h
#endif

#include <vtkm/Types.h>
#include <vtkm/internal/brigand.hpp>

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
using ListBase = brigand::list<T...>;

/// list value that is used to represent a list actually matches all values
struct UniversalTag
{
  //We never want this tag constructed, and by deleting the constructor
  //we get an error when trying to use this class with ForEach.
  UniversalTag() = delete;
};

//-----------------------------------------------------------------------------
template <typename ListTag1, typename ListTag2>
struct ListJoin
{
  using type = brigand::append<ListTag1, ListTag2>;
};

template <typename ListTag>
struct ListJoin<vtkm::detail::ListBase<vtkm::detail::UniversalTag>, ListTag>
{
  using type = vtkm::detail::ListBase<vtkm::detail::UniversalTag>;
};

template <typename ListTag>
struct ListJoin<ListTag, vtkm::detail::ListBase<vtkm::detail::UniversalTag>>
{
  using type = vtkm::detail::ListBase<vtkm::detail::UniversalTag>;
};

//-----------------------------------------------------------------------------
template <typename Type, typename List>
struct ListContainsImpl;

//-----------------------------------------------------------------------------
template <typename Type>
struct ListContainsImpl<Type, brigand::empty_sequence>
{
  static VTKM_CONSTEXPR bool value = false;
};

//-----------------------------------------------------------------------------
template <typename Type>
struct ListContainsImpl<Type, brigand::list<vtkm::detail::UniversalTag>>
{
  static VTKM_CONSTEXPR bool value = true;
};

//-----------------------------------------------------------------------------
template <typename Type, typename T1>
struct ListContainsImpl<Type, brigand::list<T1>>
{
  static VTKM_CONSTEXPR bool value = std::is_same<Type, T1>::value;
};

//-----------------------------------------------------------------------------
template <typename Type, typename T1, typename T2>
struct ListContainsImpl<Type, brigand::list<T1, T2>>
{
  static VTKM_CONSTEXPR bool value = std::is_same<Type, T1>::value || std::is_same<Type, T2>::value;
};

//-----------------------------------------------------------------------------
template <typename Type, typename T1, typename T2, typename T3>
struct ListContainsImpl<Type, brigand::list<T1, T2, T3>>
{
  static VTKM_CONSTEXPR bool value =
    std::is_same<Type, T1>::value || std::is_same<Type, T2>::value || std::is_same<Type, T3>::value;
};

//-----------------------------------------------------------------------------
template <typename Type, typename T1, typename T2, typename T3, typename T4>
struct ListContainsImpl<Type, brigand::list<T1, T2, T3, T4>>
{
  static VTKM_CONSTEXPR bool value = std::is_same<Type, T1>::value ||
    std::is_same<Type, T2>::value || std::is_same<Type, T3>::value || std::is_same<Type, T4>::value;
};

//-----------------------------------------------------------------------------
template <typename Type, typename List>
struct ListContainsImpl
{
  using find_result = brigand::find<List, std::is_same<brigand::_1, Type>>;
  using size = brigand::size<find_result>;
  static VTKM_CONSTEXPR bool value = (size::value != 0);
};

//-----------------------------------------------------------------------------
template <class T, class U, class ListTag>
struct intersect_tags
{
  using has_u = ListContainsImpl<U, ListTag>;
  using type = typename std::conditional<has_u::value, brigand::push_back<T, U>, T>::type;
};

//-----------------------------------------------------------------------------
template <typename ListTag1, typename ListTag2>
struct ListIntersect
{
  using type =
    brigand::fold<ListTag1,
                  brigand::list<>,
                  intersect_tags<brigand::_state, brigand::_element, brigand::pin<ListTag2>>>;
};

template <typename ListTag>
struct ListIntersect<vtkm::detail::ListBase<vtkm::detail::UniversalTag>, ListTag>
{
  using type = ListTag;
};

template <typename ListTag>
struct ListIntersect<ListTag, vtkm::detail::ListBase<vtkm::detail::UniversalTag>>
{
  using type = ListTag;
};

template <typename SameListTag>
struct ListIntersect<SameListTag, SameListTag>
{
  using type = SameListTag;
};

template <typename Functor>
VTKM_CONT void ListForEachImpl(Functor&&, brigand::empty_sequence)
{
}

template <typename Functor, typename T1>
VTKM_CONT void ListForEachImpl(Functor&& f, brigand::list<T1>)
{
  f(T1{});
}

template <typename Functor, typename T1, typename T2>
VTKM_CONT void ListForEachImpl(Functor&& f, brigand::list<T1, T2>)
{
  f(T1{});
  f(T2{});
}

template <typename Functor, typename T1, typename T2, typename T3>
VTKM_CONT void ListForEachImpl(Functor&& f, brigand::list<T1, T2, T3>)
{
  f(T1{});
  f(T2{});
  f(T3{});
}

template <typename Functor,
          typename T1,
          typename T2,
          typename T3,
          typename T4,
          typename... ArgTypes>
VTKM_CONT void ListForEachImpl(Functor&& f, brigand::list<T1, T2, T3, T4, ArgTypes...>)
{
  f(T1{});
  f(T2{});
  f(T3{});
  f(T4{});
  ListForEachImpl(f, brigand::list<ArgTypes...>());
}

} // namespace detail

//-----------------------------------------------------------------------------
/// A basic tag for a list of typenames. This struct can be subclassed
/// and still behave like a list tag.
template <typename... ArgTypes>
struct ListTagBase : detail::ListRoot
{
  using list = detail::ListBase<ArgTypes...>;
};
}

#endif //vtk_m_internal_ListTagDetail_h
