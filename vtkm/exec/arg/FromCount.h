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
#ifndef vtk_m_exec_arg_FromCount_h
#define vtk_m_exec_arg_FromCount_h

#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <vtkm/exec/arg/ThreadIndicesTopologyMap.h>

namespace vtkm {
namespace exec {
namespace arg {

/// \brief Aspect tag to use for getting the from count.
///
/// The \c AspectTagFromCount aspect tag causes the \c Fetch class to obtain
/// the number of indices that map to the current topology element.
///
struct AspectTagFromCount {  };

/// \brief The \c ExecutionSignature tag to get the number of from elements.
///
/// In a topology map, there are \em from and \em to topology elements
/// specified. The scheduling occurs on the \em to elements, and for each \em
/// to element there is some number of incident \em from elements that are
/// accessible. This \c ExecutionSignature tag provides the number of these \em
/// from elements that are accessible.
///
struct FromCount : vtkm::exec::arg::ExecutionSignatureTagBase
{
  static const vtkm::IdComponent INDEX = 1;
  typedef vtkm::exec::arg::AspectTagFromCount AspectTag;
};

template<typename FetchTag,
         typename ConnectivityType,
         typename ExecObjectType>
struct Fetch<FetchTag,
             vtkm::exec::arg::AspectTagFromCount,
             vtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>,
             ExecObjectType>
{
  typedef vtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>
      ThreadIndicesType;

  typedef vtkm::IdComponent ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_EXPORT
  ValueType Load(const ThreadIndicesType &indices, const ExecObjectType &) const
  {
    return indices.GetIndicesFrom().GetNumberOfComponents();
  }

  VTKM_EXEC_EXPORT
  void Store(const ThreadIndicesType &,
             const ExecObjectType &,
             const ValueType &) const
  {
    // Store is a no-op.
  }
};

}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FromCount_h
