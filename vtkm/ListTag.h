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
//  Copyright 2014. Los Alamos National Security
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

#include <vtkm/internal/ExportMacros.h>

namespace vtkm {

namespace detail {

template<typename ListTag1, typename ListTag2>
struct ListJoin { };

} // namespace detail

/// A special tag for an empty list.
///
struct ListTagEmpty {
  typedef detail::ListBase<void()> List;
};

/// A tag that is a construction of two other tags joined together. This struct
/// can be subclassed and still behave like a list tag.
template<typename ListTag1, typename ListTag2>
struct ListTagJoin {
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
  detail::ListForEachImpl(f, typename ListTag::List());
}

/// For each typename represented by the list tag, call the functor with a
/// default instance of that type.
///
template<typename Functor, typename ListTag>
VTKM_CONT_EXPORT
void ListForEach(const Functor &f, ListTag)
{
  detail::ListForEachImpl(f, typename ListTag::List());
}

} // namespace vtkm

#endif //vtk_m_ListTag_h
