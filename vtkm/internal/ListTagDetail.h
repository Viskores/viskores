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
template<typename Type, typename List> struct ListContainsImpl;

//-----------------------------------------------------------------------------
template<typename Type>
struct ListContainsImpl<Type, brigand::empty_sequence >
{
  static VTKM_CONSTEXPR bool value = false;
};

//-----------------------------------------------------------------------------
template<typename Type,
         typename T1>
struct ListContainsImpl<Type, brigand::list<T1> >
{
  static VTKM_CONSTEXPR bool value = std::is_same< Type, T1 >::value;
};

//-----------------------------------------------------------------------------
template<typename Type,
         typename T1,
         typename T2>
struct ListContainsImpl<Type, brigand::list<T1,T2> >
{
  static VTKM_CONSTEXPR bool value = std::is_same< Type, T1 >::value ||
                                     std::is_same< Type, T2 >::value;
};

//-----------------------------------------------------------------------------
template<typename Type,
         typename T1,
         typename T2,
         typename T3>
struct ListContainsImpl<Type, brigand::list<T1,T2,T3> >
{
  static VTKM_CONSTEXPR bool value = std::is_same< Type, T1 >::value ||
                                     std::is_same< Type, T2 >::value ||
                                     std::is_same< Type, T3 >::value;
};

//-----------------------------------------------------------------------------
template<typename Type,
         typename T1,
         typename T2,
         typename T3,
         typename T4>
struct ListContainsImpl<Type, brigand::list<T1,T2,T3,T4> >
{
  static VTKM_CONSTEXPR bool value = std::is_same< Type, T1 >::value ||
                                     std::is_same< Type, T2 >::value ||
                                     std::is_same< Type, T3 >::value ||
                                     std::is_same< Type, T4 >::value;
};

//-----------------------------------------------------------------------------
template<typename Type, typename List> struct ListContainsImpl
{
  using find_result = brigand::find< List,
                                     std::is_same< brigand::_1, Type> >;
  using size = brigand::size<find_result>;
  static VTKM_CONSTEXPR bool value = (size::value != 0);
};

//-----------------------------------------------------------------------------
template<typename Functor>
VTKM_CONT
void ListForEachImpl(const Functor &, brigand::empty_sequence)
{
}

template<typename Functor,
         typename T1>
VTKM_CONT
void ListForEachImpl(const Functor &f, brigand::list<T1>)
{
  f(T1());
}

template<typename Functor,
         typename T1,
         typename T2>
VTKM_CONT
void ListForEachImpl(const Functor &f, brigand::list<T1,T2>)
{
  f(T1());
  f(T2());
}

template<typename Functor,
         typename T1,
         typename T2,
         typename T3>
VTKM_CONT
void ListForEachImpl(const Functor &f, brigand::list<T1,T2,T3>)
{
  f(T1());
  f(T2());
  f(T3());
}

template<typename Functor,
         typename T1,
         typename T2,
         typename T3,
         typename T4,
         typename... ArgTypes>
VTKM_CONT
void ListForEachImpl(const Functor &f, brigand::list<T1,T2,T3,T4,ArgTypes...>)
{
  f(T1());
  f(T2());
  f(T3());
  f(T4());
  ListForEachImpl(f, brigand::list<ArgTypes...>());
}

template<typename Functor>
VTKM_CONT
void ListForEachImpl(Functor &, brigand::empty_sequence)
{
}

template<typename Functor,
         typename T1>
VTKM_CONT
void ListForEachImpl(Functor &f, brigand::list<T1>)
{
  f(T1());
}

template<typename Functor,
         typename T1,
         typename T2>
VTKM_CONT
void ListForEachImpl(Functor &f, brigand::list<T1,T2>)
{
  f(T1());
  f(T2());
}

template<typename Functor,
         typename T1,
         typename T2,
         typename T3>
VTKM_CONT
void ListForEachImpl(Functor &f, brigand::list<T1,T2,T3>)
{
  f(T1());
  f(T2());
  f(T3());
}

template<typename Functor,
         typename T1,
         typename T2,
         typename T3,
         typename T4,
         typename... ArgTypes>
VTKM_CONT
void ListForEachImpl(Functor &f, brigand::list<T1,T2,T3,T4,ArgTypes...>)
{
  f(T1());
  f(T2());
  f(T3());
  f(T4());
  ListForEachImpl(f, brigand::list<ArgTypes...>());
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
