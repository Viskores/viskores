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
#ifndef vtk_m_exec_arg_FetchTagArrayTopologyMapIn_h
#define vtk_m_exec_arg_FetchTagArrayTopologyMapIn_h

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>

namespace vtkm {
namespace exec {
namespace arg {

/// \brief \c Fetch tag for getting array values with direct indexing.
///
/// \c FetchTagArrayTopologyMapIn is a tag used with the \c Fetch class to
/// retreive values from an array portal. The fetch uses indexing based on
/// the topology structure used for the input domain.
///
template <vtkm::IdComponent nvals>
struct FetchTagArrayTopologyMapIn {  };

template<typename Invocation,
         vtkm::IdComponent ParameterIndex,
         vtkm::IdComponent nvals>
struct Fetch<
    vtkm::exec::arg::FetchTagArrayTopologyMapIn<nvals>,
    vtkm::exec::arg::AspectTagDefault,
    Invocation,
    ParameterIndex>
{
  typedef typename Invocation::ParameterInterface::
      template ParameterType<ParameterIndex>::type ExecObjectType;

  typedef vtkm::Vec<typename ExecObjectType::ValueType,nvals> ValueType;

  VTKM_EXEC_EXPORT
  ValueType Load(vtkm::Id index, const Invocation &invocation) const
  {
    static const vtkm::IdComponent InputDomainIndex =
        Invocation::InputDomainIndex;
    typedef typename Invocation::ParameterInterface ParameterInterface;
    typedef typename ParameterInterface::
        template ParameterType<InputDomainIndex>::type TopologyType;
    TopologyType topology =
        invocation.Parameters.template GetParameter<InputDomainIndex>();


    int nids = topology.GetNumberOfIndices(index);
    
    vtkm::Vec<vtkm::Id,nvals> ids;
    topology.GetIndices(index,ids);

    ValueType v;
    for (int i=0; i<nids && i<nvals; ++i)
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
