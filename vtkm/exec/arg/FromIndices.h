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
#ifndef vtk_m_exec_arg_FromIndices_h
#define vtk_m_exec_arg_FromIndices_h

#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ThreadIndicesTopologyMap.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the from indices.
///
/// The \c AspectTagFromIndices aspect tag causes the \c Fetch class to obtain
/// the indices that map to the current topology element.
///
struct AspectTagFromIndices
{
};

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
  static constexpr vtkm::IdComponent INDEX = 1;
  using AspectTag = vtkm::exec::arg::AspectTagFromIndices;
};

template <typename FetchTag, typename ConnectivityType, typename ExecObjectType>
struct Fetch<FetchTag,
             vtkm::exec::arg::AspectTagFromIndices,
             vtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>,
             ExecObjectType>
{
  using ThreadIndicesType = vtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

  using ValueType = typename ThreadIndicesType::IndicesFromType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetIndicesFrom();
  }

  VTKM_EXEC
  void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op.
  }
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FromIndices_h
