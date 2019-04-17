//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_ThreadIndices_h
#define vtk_m_exec_arg_ThreadIndices_h

#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <vtkm/exec/arg/Fetch.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the thread indices.
///
/// The \c AspectTagThreadIndices aspect tag causes the \c Fetch class to
/// ignore whatever data is in the associated execution object and return the
/// thread indices.
///
struct AspectTagThreadIndices
{
};

/// \brief The \c ExecutionSignature tag to use to get the thread indices
///
/// When a worklet is dispatched, it broken into pieces defined by the input
/// domain and scheduled on independent threads. During this process multiple
/// indices associated with the input and output can be generated. This tag in
/// the \c ExecutionSignature passes the index for this work. \c WorkletBase
/// contains a typedef that points to this class.
///
struct ThreadIndices : vtkm::exec::arg::ExecutionSignatureTagBase
{
  // The index does not really matter because the fetch is going to ignore it.
  // However, it still has to point to a valid parameter in the
  // ControlSignature because the templating is going to grab a fetch tag
  // whether we use it or not. 1 should be guaranteed to be valid since you
  // need at least one argument for the input domain.
  static constexpr vtkm::IdComponent INDEX = 1;
  using AspectTag = vtkm::exec::arg::AspectTagThreadIndices;
};

template <typename FetchTag, typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<FetchTag, vtkm::exec::arg::AspectTagThreadIndices, ThreadIndicesType, ExecObjectType>
{
  using ValueType = const ThreadIndicesType&;

  VTKM_EXEC
  const ThreadIndicesType& Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices;
  }

  VTKM_EXEC
  void Store(const ThreadIndicesType&, const ExecObjectType&, const ThreadIndicesType&) const
  {
    // Store is a no-op.
  }
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_ThreadIndices_h
