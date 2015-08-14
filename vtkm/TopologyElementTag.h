//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_TopologyElementTag_h
#define vtk_m_TopologyElementTag_h

#include <vtkm/Types.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/mpl/assert.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {

/// \brief A tag used to identify the cell elements in a topology.
///
/// A topology element refers to some type of substructure of a topology. For
/// example, a 3D mesh has points, edges, faces, and cells. Each of these is an
/// example of a topology element and has its own tag.
///
struct TopologyElementTagCell {  };

/// \brief A tag used to identify the point elements in a topology.
///
/// A topology element refers to some type of substructure of a topology. For
/// example, a 3D mesh has points, edges, faces, and cells. Each of these is an
/// example of a topology element and has its own tag.
///
struct TopologyElementTagPoint {  };

/// \brief A tag used to identify the edge elements in a topology.
///
/// A topology element refers to some type of substructure of a topology. For
/// example, a 3D mesh has points, edges, faces, and cells. Each of these is an
/// example of a topology element and has its own tag.
///
struct TopologyElementTagEdge {  };

/// \brief A tag used to identify the face elements in a topology.
///
/// A topology element refers to some type of substructure of a topology. For
/// example, a 3D mesh has points, edges, faces, and cells. Each of these is an
/// example of a topology element and has its own tag.
///
struct TopologyElementTagFace {  };


namespace internal {

/// Checks to see if the given object is a topology element tag. This check is
/// compatible with the Boost meta-template programing library (MPL). It
/// contains a typedef named \c type that is either boost::mpl::true_ or
/// boost::mpl::false_. Both of these have a typedef named value with the
/// respective boolean value.
///
template<typename T>
struct TopologyElementTagCheck
{
  typedef boost::mpl::false_ type;
};

template<>
struct TopologyElementTagCheck<vtkm::TopologyElementTagCell>
{
  typedef boost::mpl::true_ type;
};

template<>
struct TopologyElementTagCheck<vtkm::TopologyElementTagPoint>
{
  typedef boost::mpl::true_ type;
};

template<>
struct TopologyElementTagCheck<vtkm::TopologyElementTagEdge>
{
  typedef boost::mpl::true_ type;
};

template<>
struct TopologyElementTagCheck<vtkm::TopologyElementTagFace>
{
  typedef boost::mpl::true_ type;
};

#define VTKM_IS_TOPOLOGY_ELEMENT_TAG(type) \
  BOOST_MPL_ASSERT(( ::vtkm::internal::TopologyElementTagCheck<type> ))

} // namespace internal

} // namespace vtkm

#endif //vtk_m_TopologyElementTag_h
