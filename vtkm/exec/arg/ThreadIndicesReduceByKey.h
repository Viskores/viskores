//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_arg_ThreadIndicesReduceByKey_h
#define vtk_m_exec_arg_ThreadIndicesReduceByKey_h

#include <vtkm/exec/arg/ThreadIndicesBasic.h>

#include <vtkm/exec/internal/ReduceByKeyLookup.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief Container for thread indices in a reduce by key invocation.
///
/// This specialization of \c ThreadIndices adds extra indices that deal with a
/// reduce by key. In particular, it save the indices used to map from a unique
/// key index to the group of input values that has that key associated with
/// it.
///
class ThreadIndicesReduceByKey : public vtkm::exec::arg::ThreadIndicesBasic
{
  using Superclass = vtkm::exec::arg::ThreadIndicesBasic;

public:
  template <typename P1, typename P2, typename P3>
  VTKM_EXEC ThreadIndicesReduceByKey(
    vtkm::Id threadIndex,
    vtkm::Id inIndex,
    vtkm::IdComponent visitIndex,
    const vtkm::exec::internal::ReduceByKeyLookup<P1, P2, P3>& keyLookup,
    vtkm::Id globalThreadIndexOffset = 0)
    : Superclass(threadIndex, inIndex, visitIndex, globalThreadIndexOffset)
    , ValueOffset(keyLookup.Offsets.Get(inIndex))
    , NumberOfValues(keyLookup.Counts.Get(inIndex))
  {
  }

  VTKM_EXEC
  vtkm::Id GetValueOffset() const { return this->ValueOffset; }

  VTKM_EXEC
  vtkm::IdComponent GetNumberOfValues() const { return this->NumberOfValues; }

private:
  vtkm::Id ValueOffset;
  vtkm::IdComponent NumberOfValues;
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_ThreadIndicesReduceByKey_h
