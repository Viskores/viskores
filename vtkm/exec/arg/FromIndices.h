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
#ifndef vtk_m_exec_arg_FromIndices_h
#define vtk_m_exec_arg_FromIndices_h

#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>

namespace vtkm {
namespace exec {
namespace arg {

/// \brief Aspect tag to use for getting the from indices.
///
/// The \c AspectTagFromIndices aspect tag causes the \c Fetch class to obtain
/// the indices that map to the current topology element.
///
struct AspectTagFromIndices {  };

/// \brief The \c ExecutionSignature tag to get the indices of from elements.
///
/// In a topology map, there are \em from and \em to topology elements
/// specified. The scheduling occurs on the \em to elements, and for each \em
/// to element there is some number of incident \em from elements that are
/// accessible. This \c ExecutionSignature tag provides the indices of these
/// \em from elements that are accessible.
///
struct FromIndices : vtkm::exec::arg::ExecutionSignatureTagBase
{
  static const vtkm::IdComponent INDEX = 1;
  typedef vtkm::exec::arg::AspectTagFromIndices AspectTag;
};

template<typename FetchTag,
         typename Invocation,
         vtkm::IdComponent ParameterIndex>
struct Fetch<
    FetchTag, vtkm::exec::arg::AspectTagFromIndices, Invocation, ParameterIndex>
{
  // The parameter for the input domain is stored in the Invocation. (It is
  // also in the worklet, but it is safer to get it from the Invocation
  // in case some other dispatch operation had to modify it.)
  static const vtkm::IdComponent InputDomainIndex =
      Invocation::InputDomainIndex;

  // Assuming that this fetch is used in a topology map, which is its
  // intention, InputDomainIndex points to a connectivity object. Thus,
  // ConnectivityType is one of the vtkm::exec::Connectivity* classes.
  typedef typename Invocation::ParameterInterface::
      template ParameterType<InputDomainIndex>::type ConnectivityType;

  typedef typename ConnectivityType::IndicesType ValueType;

  VTKM_EXEC_EXPORT
  ValueType Load(vtkm::Id index, const Invocation &invocation) const
  {
    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const ConnectivityType &connectivity =
        invocation.Parameters.template GetParameter<InputDomainIndex>();

    return connectivity.GetIndices(index);
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

#endif //vtk_m_exec_arg_FromIndices_h
