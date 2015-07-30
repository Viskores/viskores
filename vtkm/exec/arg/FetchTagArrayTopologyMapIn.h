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
#ifndef vtk_m_exec_arg_FetchTagArrayTopologyMapIn_h
#define vtk_m_exec_arg_FetchTagArrayTopologyMapIn_h

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/TopologyData.h>

VTKM_BOOST_PRE_INCLUDE
#include <boost/type_traits.hpp>
VTKM_BOOST_POST_INCLUDE

namespace vtkm {
namespace exec {
namespace arg {

/// \brief \c Fetch tag for getting array values with direct indexing.
///
/// \c FetchTagArrayTopologyMapIn is a tag used with the \c Fetch class to
/// retreive values from an array portal. The fetch uses indexing based on
/// the topology structure used for the input domain.
///
struct FetchTagArrayTopologyMapIn {  };

template<typename Invocation, vtkm::IdComponent ParameterIndex>
struct Fetch<
    vtkm::exec::arg::FetchTagArrayTopologyMapIn,
    vtkm::exec::arg::AspectTagDefault,
    Invocation,
    ParameterIndex>
{
  static const vtkm::IdComponent InputDomainIndex =
      Invocation::InputDomainIndex;

  typedef typename Invocation::ControlInterface::template
      ParameterType<InputDomainIndex>::type ControlSignatureTag;

  static const vtkm::IdComponent ITEM_TUPLE_LENGTH =
      ControlSignatureTag::ITEM_TUPLE_LENGTH;

  typedef typename Invocation::ParameterInterface::
      template ParameterType<ParameterIndex>::type ExecObjectType;

  typedef boost::remove_const<typename ExecObjectType::ValueType> NonConstType;
  typedef vtkm::exec::TopologyData<typename NonConstType::type,
                                   ITEM_TUPLE_LENGTH> ValueType;

  VTKM_EXEC_EXPORT
  ValueType Load(vtkm::Id index, const Invocation &invocation) const
  {
    typedef typename Invocation::ParameterInterface ParameterInterface;
    typedef typename ParameterInterface::
        template ParameterType<InputDomainIndex>::type TopologyType;
    TopologyType topology =
        invocation.Parameters.template GetParameter<InputDomainIndex>();


    vtkm::IdComponent nids =
      static_cast<vtkm::IdComponent>(topology.GetNumberOfIndices(index));

    vtkm::Vec<vtkm::Id,ITEM_TUPLE_LENGTH> ids;
    topology.GetIndices(index,ids);

    ValueType v;
    for (vtkm::IdComponent i=0; i<nids && i<ITEM_TUPLE_LENGTH; ++i)
    {
        v[i] = invocation.Parameters.template GetParameter<ParameterIndex>().
            Get(ids[i]);
    }
    return v;
  }

  VTKM_EXEC_EXPORT
  void Store(vtkm::Id, const Invocation &, const ValueType &) const
  {
    // Store is a no-op for this fetch.
  }
};

}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FetchTagArrayTopologyMapIn_h
