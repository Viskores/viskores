//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_FetchTagArrayDirectIn_h
#define vtk_m_exec_arg_FetchTagArrayDirectIn_h

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting array values with direct indexing.
///
/// \c FetchTagArrayDirectIn is a tag used with the \c Fetch class to retrieve
/// values from an array portal. The fetch uses direct indexing, so the thread
/// index given to \c Load is used as the index into the array.
///
struct FetchTagArrayDirectIn
{
};


VTKM_SUPPRESS_EXEC_WARNINGS
template <typename T, typename U>
inline VTKM_EXEC T load(const U& u, vtkm::Id v)
{
  return u.Get(v);
}

VTKM_SUPPRESS_EXEC_WARNINGS
template <typename T, typename U>
inline VTKM_EXEC T load(const U* u, vtkm::Id v)
{
  return u->Get(v);
}

template <typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<vtkm::exec::arg::FetchTagArrayDirectIn,
             vtkm::exec::arg::AspectTagDefault,
             ThreadIndicesType,
             ExecObjectType>
{
  //need to remove pointer type from ThreadIdicesType
  using ET = typename std::remove_const<typename std::remove_pointer<ExecObjectType>::type>::type;
  using PortalType =
    typename std::conditional<std::is_pointer<ExecObjectType>::value, const ET*, const ET&>::type;

  using ValueType = typename ET::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, PortalType arrayPortal) const
  {
    return load<ValueType>(arrayPortal, indices.GetInputIndex());
  }

  VTKM_EXEC
  void Store(const ThreadIndicesType&, PortalType, const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FetchTagArrayDirectIn_h
