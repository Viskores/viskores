//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_VisitIndex_h
#define vtk_m_exec_arg_VisitIndex_h

#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <vtkm/exec/arg/Fetch.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the work index.
///
/// The \c AspectTagVisitIndex aspect tag causes the \c Fetch class to ignore
/// whatever data is in the associated execution object and return the visit
/// index.
///
struct AspectTagVisitIndex
{
};

/// \brief The \c ExecutionSignature tag to use to get the visit index
///
/// When a worklet is dispatched, there is a scatter operation defined that
/// optionally allows each input to go to multiple output entries. When one
/// input is assigned to multiple outputs, there needs to be a mechanism to
/// uniquely identify which output is which. The visit index is a value between
/// 0 and the number of outputs a particular input goes to. This tag in the \c
/// ExecutionSignature passes the visit index for this work. \c WorkletBase
/// contains a typedef that points to this class.
///
struct VisitIndex : vtkm::exec::arg::ExecutionSignatureTagBase
{
  // The index does not really matter because the fetch is going to ignore it.
  // However, it still has to point to a valid parameter in the
  // ControlSignature because the templating is going to grab a fetch tag
  // whether we use it or not. 1 should be guaranteed to be valid since you
  // need at least one argument for the input domain.
  static constexpr vtkm::IdComponent INDEX = 1;
  using AspectTag = vtkm::exec::arg::AspectTagVisitIndex;
};

template <typename FetchTag, typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<FetchTag, vtkm::exec::arg::AspectTagVisitIndex, ThreadIndicesType, ExecObjectType>
{
  using ValueType = vtkm::IdComponent;

  VTKM_EXEC
  vtkm::IdComponent Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetVisitIndex();
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

#endif //vtk_m_exec_arg_VisitIndex_h
