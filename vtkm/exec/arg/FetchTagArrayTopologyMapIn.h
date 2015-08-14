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

#include <vtkm/exec/internal/VecFromPortalPermute.h>

namespace vtkm {
namespace exec {
namespace arg {

/// \brief \c Fetch tag for getting array values determined by topology connections.
///
/// \c FetchTagArrayTopologyMapIn is a tag used with the \c Fetch class to
/// retreive values from an array portal. The fetch uses indexing based on
/// the topology structure used for the input domain.
///
struct FetchTagArrayTopologyMapIn {  };

namespace detail {

// This internal class defines how a TopologyMapIn fetch loads from field data
// based on the connectivity class and the object holding the field data. The
// default implementation gets a Vec of indices and an array portal for the
// field and delivers a VecFromPortalPermute. Specializations could have more
// efficient implementations. For example, if the connectivity is structured
// and the field is regular point coordinates, it is much faster to compute the
// field directly.

template<typename ConnectivityType, typename FieldExecObjectType>
struct FetchArrayTopologyMapInImplementation
{
  // The connectivity classes are expected to have an IndicesType that is
  // is Vec-like class that will be returned from a GetIndices method.
  typedef typename ConnectivityType::IndicesType IndexVecType;

  // The FieldExecObjectType is expected to behave like an ArrayPortal.
  typedef FieldExecObjectType PortalType;

  typedef vtkm::exec::internal::VecFromPortalPermute<
      IndexVecType,PortalType> ValueType;

  VTKM_EXEC_EXPORT
  static ValueType Load(vtkm::Id index,
                        const ConnectivityType &connectivity,
                        const FieldExecObjectType &field)
  {
    return ValueType(connectivity.GetIndices(index), field);
  }
};

} // namespace detail

template<typename Invocation, vtkm::IdComponent ParameterIndex>
struct Fetch<
    vtkm::exec::arg::FetchTagArrayTopologyMapIn,
    vtkm::exec::arg::AspectTagDefault,
    Invocation,
    ParameterIndex>
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

  // The execution object associated with this parameter is expected to be
  // an array portal containing the field values.
  typedef typename Invocation::ParameterInterface::
      template ParameterType<ParameterIndex>::type ExecObjectType;

  typedef detail::FetchArrayTopologyMapInImplementation<
      ConnectivityType,ExecObjectType> Implementation;

  typedef typename Implementation::ValueType ValueType;

  VTKM_EXEC_EXPORT
  ValueType Load(vtkm::Id index, const Invocation &invocation) const
  {
    const ConnectivityType &connectivity =
        invocation.Parameters.template GetParameter<InputDomainIndex>();
    const ExecObjectType &field =
        invocation.Parameters.template GetParameter<ParameterIndex>();

    return Implementation::Load(index, connectivity, field);
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
