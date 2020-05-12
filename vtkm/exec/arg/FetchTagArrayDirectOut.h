//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_FetchTagArrayDirectOut_h
#define vtk_m_exec_arg_FetchTagArrayDirectOut_h

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for setting array values with direct indexing.
///
/// \c FetchTagArrayDirectOut is a tag used with the \c Fetch class to store
/// values in an array portal. The fetch uses direct indexing, so the thread
/// index given to \c Store is used as the index into the array.
///
struct FetchTagArrayDirectOut
{
};

template <typename ExecObjectType>
struct Fetch<vtkm::exec::arg::FetchTagArrayDirectOut,
             vtkm::exec::arg::AspectTagDefault,
             ExecObjectType>
{
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VTKM_EXEC auto Load(const ThreadIndicesType&, const ExecObjectType&) const ->
    typename ExecObjectType::ValueType
  {
    // Load is a no-op for this fetch.
    using ValueType = typename ExecObjectType::ValueType;
    return ValueType();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType, typename T>
  VTKM_EXEC void Store(const ThreadIndicesType& indices,
                       const ExecObjectType& arrayPortal,
                       const T& value) const
  {
    using ValueType = typename ExecObjectType::ValueType;
    arrayPortal.Set(indices.GetOutputIndex(), static_cast<ValueType>(value));
  }
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FetchTagArrayDirectOut_h
