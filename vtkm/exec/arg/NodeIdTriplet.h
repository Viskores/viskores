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
#ifndef vtk_m_exec_arg_NodeIdTriplet_h
#define vtk_m_exec_arg_NodeIdTriplet_h

#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>

namespace vtkm {
namespace exec {
namespace arg {

/// \brief Aspect tag to use for getting the work index.
///
/// The \c AspectTagNodeIdTriplet aspect tag causes the \c Fetch class to
/// obtain the first three node IDs for a cell from a topology object.
///
struct AspectTagNodeIdTriplet {  };

/// \brief The \c ExecutionSignature tag to use to get the node IDs.
///
struct NodeIdTriplet : vtkm::exec::arg::ExecutionSignatureTagBase
{
  static const vtkm::IdComponent INDEX = 1;
  typedef vtkm::exec::arg::AspectTagNodeIdTriplet AspectTag;
};

template<typename FetchTag, typename Invocation>
struct Fetch<FetchTag, vtkm::exec::arg::AspectTagNodeIdTriplet, Invocation, 1>
{
  typedef vtkm::Id3 ValueType;

  VTKM_EXEC_EXPORT
  vtkm::Id3 Load(vtkm::Id index, const Invocation &invocation) const
  {
    // The parameter for the input domain is stored in the Invocation. (It is
    // also in the worklet, but it is safer to get it from the Invocation
    // in case some other dispatch operation had to modify it.)
    static const vtkm::IdComponent InputDomainIndex =
        Invocation::InputDomainIndex;

    // ParameterInterface (from Invocation) is a FunctionInterface type
    // containing types for all objects passed to the Invoke method (with
    // some dynamic casting performed so objects like DynamicArrayHandle get
    // cast to ArrayHandle).
    typedef typename Invocation::ParameterInterface ParameterInterface;

    // This is the type for the input domain (derived from the last two things
    // we got from the Invocation).
    typedef typename ParameterInterface::
        template ParameterType<InputDomainIndex>::type TopologyType;

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    TopologyType topology =
        invocation.Parameters.template GetParameter<InputDomainIndex>();

    return vtkm::Id3(topology.Connectivity.GetPortalControl().Get(index*3+0),
                     topology.Connectivity.GetPortalControl().Get(index*3+1),
                     topology.Connectivity.GetPortalControl().Get(index*3+2));
  }

  VTKM_EXEC_EXPORT
  void Store(vtkm::Id, const Invocation &, const ValueType &) const
  {
    // Store is a no-op.
  }
};

}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_NodeIdTriplet_h
