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

#ifndef vtk_m_internal_ListTagDetail_h
#define vtk_m_internal_ListTagDetail_h

#if !defined(vtk_m_ListTag_h) && !defined(VTKM_TEST_HEADER_BUILD)
#error ListTagDetail.h must be included from ListTag.h
#endif

#include <vtkm/Types.h>
#include <vtkm/internal/brigand.hpp>

namespace vtkm {
namespace detail {

//-----------------------------------------------------------------------------

/// Base class that all ListTag classes inherit from. Helps identify lists
/// in macros like VTKM_IS_LIST_TAG.
///
struct ListRoot {  };

template <class... T>
using ListBase = brigand::list<T...>;

//-----------------------------------------------------------------------------
template<typename ListTag1, typename ListTag2>
struct ListJoin
{
  using type = brigand::append< typename ListTag1::list, typename ListTag2::list>;
};


//-----------------------------------------------------------------------------
template<typename Type, typename List> struct ListContainsImpl
{
  using find_result = brigand::find< List,
                                     std::is_same< brigand::_1, Type> >;
  using size = brigand::size<find_result>;
  static constexpr bool value = (size::value != 0);
};

//-----------------------------------------------------------------------------
template<typename Functor>
VTKM_CONT_EXPORT
void ListForEachImpl(const Functor &, brigand::empty_sequence) {  }

template<typename Functor, typename... ArgTypes>
VTKM_CONT_EXPORT
void ListForEachImpl(const Functor &f, brigand::list<ArgTypes...>)
{
  brigand::for_each_args( f, ArgTypes()... );
}

template< typename Functor>
struct func_wrapper
{
  Functor& f;

  func_wrapper(Functor& func): f(func) {}

  template<typename T>
  void operator()(T&& t)
    {
    f(std::forward<T>(t));
    }
};

template<typename Functor, typename... ArgTypes>
VTKM_CONT_EXPORT
void ListForEachImpl(Functor &f, brigand::list<ArgTypes...>)
{
  func_wrapper<Functor> wrapper(f);
  brigand::for_each_args( wrapper, ArgTypes()... );
}

} // namespace detail

//-----------------------------------------------------------------------------
/// A basic tag for a list of typenames. This struct can be subclassed
/// and still behave like a list tag.
template<typename... ArgTypes>
struct ListTagBase : detail::ListRoot
{
  using list = detail::ListBase<ArgTypes...>;
};

}

#endif //vtk_m_internal_ListTagDetail_h
