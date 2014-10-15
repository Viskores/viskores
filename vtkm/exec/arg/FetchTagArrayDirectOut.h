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
#ifndef vtk_m_exec_arg_FetchTagArrayDirectOut_h
#define vtk_m_exec_arg_FetchTagArrayDirectOut_h

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>

namespace vtkm {
namespace exec {
namespace arg {

/// \brief \c Fetch tag for setting array values with direct indexing.
///
/// \c FetchTagArrayDirectOut is a tag used with the \c Fetch class to store
/// values in an array portal. The fetch uses direct indexing, so the thread
/// index given to \c Store is used as the index into the array.
///
struct FetchTagArrayDirectOut {  };


template<typename Invocation, vtkm::IdComponent ParameterIndex>
struct Fetch<
    vtkm::exec::arg::FetchTagArrayDirectOut,
    vtkm::exec::arg::AspectTagDefault,
    Invocation,
    ParameterIndex>
{
  typedef typename Invocation::ParameterInterface::
      template ParameterType<ParameterIndex>::type ExecObjectType;

  typedef typename ExecObjectType::ValueType ValueType;

  VTKM_EXEC_EXPORT
  ValueType Load(vtkm::Id, const Invocation &) const
  {
    // Load is a no-op for this fetch.
    return ValueType();
  }

  VTKM_EXEC_EXPORT
  void Store(vtkm::Id index,
             const Invocation &invocation,
             const ValueType &value) const
  {
    invocation.Parameters.template GetParameter<ParameterIndex>().
        Set(index, value);
  }
};

}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FetchTagArrayDirectOut_h
