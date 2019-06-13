//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_OnBoundary_h
#define vtk_m_exec_arg_OnBoundary_h

#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ThreadIndicesPointNeighborhood.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting if a point is a boundary point.
///
/// The \c AspectTagBoundary aspect tag causes the \c Fetch class to obtain
/// if the point is on a boundary.
///
struct AspectTagBoundary
{
};


/// \brief The \c ExecutionSignature tag to get if executing on a boundary element
///
struct Boundary : vtkm::exec::arg::ExecutionSignatureTagBase
{
  static constexpr vtkm::IdComponent INDEX = 1;
  using AspectTag = vtkm::exec::arg::AspectTagBoundary;
};

template <typename FetchTag, typename ExecObjectType>
struct Fetch<FetchTag,
             vtkm::exec::arg::AspectTagBoundary,
             vtkm::exec::arg::ThreadIndicesPointNeighborhood,
             ExecObjectType>
{
  using ThreadIndicesType = vtkm::exec::arg::ThreadIndicesPointNeighborhood;

  using ValueType = vtkm::exec::BoundaryState;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetBoundaryState();
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

#endif //vtk_m_exec_arg_OnBoundary_h
