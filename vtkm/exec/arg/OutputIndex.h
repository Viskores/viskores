//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_OutputIndex_h
#define vtk_m_exec_arg_OutputIndex_h

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
/// The \c AspectTagOutputIndex aspect tag causes the \c Fetch class to ignore
/// whatever data is in the associated execution object and return the index
/// of the output element.
///
struct AspectTagOutputIndex
{
};

/// \brief The \c ExecutionSignature tag to use to get the output index
///
/// When a worklet is dispatched, it broken into pieces defined by the output
/// domain and scheduled on independent threads. This tag in the \c
/// ExecutionSignature passes the index of the output element that the work
/// thread is currently working on. When a worklet has a scatter associated
/// with it, the output and output indices can be different. \c WorkletBase
/// contains a typedef that points to this class.
///
struct OutputIndex : vtkm::exec::arg::ExecutionSignatureTagBase
{
  // The index does not really matter because the fetch is going to ignore it.
  // However, it still has to point to a valid parameter in the
  // ControlSignature because the templating is going to grab a fetch tag
  // whether we use it or not. 1 should be guaranteed to be valid since you
  // need at least one argument for the output domain.
  static constexpr vtkm::IdComponent INDEX = 1;
  using AspectTag = vtkm::exec::arg::AspectTagOutputIndex;
};

template <typename FetchTag, typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<FetchTag, vtkm::exec::arg::AspectTagOutputIndex, ThreadIndicesType, ExecObjectType>
{
  using ValueType = vtkm::Id;

  VTKM_EXEC
  vtkm::Id Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetOutputIndex();
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

#endif //vtk_m_exec_arg_OutputIndex_h
