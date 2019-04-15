//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_FetchTagArrayNeighborhoodIn_h
#define vtk_m_exec_arg_FetchTagArrayNeighborhoodIn_h

#include <vtkm/exec/FieldNeighborhood.h>
#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ThreadIndicesPointNeighborhood.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting values of neighborhood around a point.
///
/// \c FetchTagArrayNeighborhoodIn is a tag used with the \c Fetch class to retrieve
/// values from an neighborhood.
///
struct FetchTagArrayNeighborhoodIn
{
};

template <typename ExecObjectType>
struct Fetch<vtkm::exec::arg::FetchTagArrayNeighborhoodIn,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::arg::ThreadIndicesPointNeighborhood,
             ExecObjectType>
{
  using ThreadIndicesType = vtkm::exec::arg::ThreadIndicesPointNeighborhood;
  using ValueType = vtkm::exec::FieldNeighborhood<ExecObjectType>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType& arrayPortal) const
  {
    return ValueType(arrayPortal, indices.GetBoundaryState());
  }

  VTKM_EXEC
  void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FetchTagArrayNeighborhoodIn_h
