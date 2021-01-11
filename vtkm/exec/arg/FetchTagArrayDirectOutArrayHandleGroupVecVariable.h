//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_FetchTagArrayDirectOutArrayHandleGroupVecVariable_h
#define vtk_m_exec_arg_FetchTagArrayDirectOutArrayHandleGroupVecVariable_h

#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>

// We need to override the fetch for output fields using
// ArrayPortalGroupVecVariable because this portal does not behave like most
// ArrayPortals. Usually you ignore the Load and implement the Store. But if
// you ignore the Load, the VecFromPortal gets no portal to set values into.
// Instead, you need to implement the Load to point to the array portal. You
// can also ignore the Store because the data is already set in the array at
// that point.

// This file is included from ArrayHandleGroupVecVariable.h

namespace vtkm
{
namespace exec
{
namespace arg
{

// We need to override the fetch for output fields using
// ArrayPortalGroupVecVariable because this portal does not behave like most
// ArrayPortals. Usually you ignore the Load and implement the Store. But if
// you ignore the Load, the VecFromPortal gets no portal to set values into.
// Instead, you need to implement the Load to point to the array portal. You
// can also ignore the Store because the data is already set in the array at
// that point.
template <typename ComponentsPortalType, typename OffsetsPortalType>
struct Fetch<vtkm::exec::arg::FetchTagArrayDirectOut,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::internal::ArrayPortalGroupVecVariable<ComponentsPortalType, OffsetsPortalType>>
{
  using ExecObjectType =
    vtkm::internal::ArrayPortalGroupVecVariable<ComponentsPortalType, OffsetsPortalType>;
  using ValueType = typename ExecObjectType::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VTKM_EXEC ValueType Load(const ThreadIndicesType& indices,
                           const ExecObjectType& arrayPortal) const
  {
    return arrayPortal.Get(indices.GetOutputIndex());
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VTKM_EXEC void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // We can actually ignore this because the VecFromPortal will already have
    // set new values in the array.
  }
};

}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FetchTagArrayDirectOutArrayHandleGroupVecVariable_h
