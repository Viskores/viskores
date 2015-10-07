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
#ifndef vtk_m_exec_arg_FetchTagArrayDirectInOut_h
#define vtk_m_exec_arg_FetchTagArrayDirectInOut_h

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>

namespace vtkm {
namespace exec {
namespace arg {

/// \brief \c Fetch tag for in-place modifying array values with direct indexing.
///
/// \c FetchTagArrayDirectInOut is a tag used with the \c Fetch class to do
/// in-place modification of values in an array portal. The fetch uses direct
/// indexing, so the thread index given to \c Store is used as the index into
/// the array.
///
struct FetchTagArrayDirectInOut {  };


template<typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<
    vtkm::exec::arg::FetchTagArrayDirectInOut,
    vtkm::exec::arg::AspectTagDefault,
    ThreadIndicesType,
    ExecObjectType>
{
  typedef typename ExecObjectType::ValueType ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_EXPORT
  ValueType Load(const ThreadIndicesType &indices,
                 const ExecObjectType &arrayPortal) const
  {
    return arrayPortal.Get(indices.GetIndex());
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_EXPORT
  void Store(const ThreadIndicesType &indices,
             const ExecObjectType &arrayPortal,
             const ValueType &value) const
  {
    arrayPortal.Set(indices.GetIndex(), value);
  }
};

}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FetchTagArrayDirectInOut_h
