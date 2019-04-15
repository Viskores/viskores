//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_FetchTagKeysIn_h
#define vtk_m_exec_arg_FetchTagKeysIn_h

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ThreadIndicesReduceByKey.h>

#include <vtkm/exec/internal/ReduceByKeyLookup.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting key values in a reduce by key.
///
/// \c FetchTagKeysIn is a tag used with the \c Fetch class to retrieve keys
/// from the input domain of a reduce by keys worklet.
///
struct FetchTagKeysIn
{
};

template <typename KeyPortalType, typename IdPortalType, typename IdComponentPortalType>
struct Fetch<
  vtkm::exec::arg::FetchTagKeysIn,
  vtkm::exec::arg::AspectTagDefault,
  vtkm::exec::arg::ThreadIndicesReduceByKey,
  vtkm::exec::internal::ReduceByKeyLookup<KeyPortalType, IdPortalType, IdComponentPortalType>>
{
  using ThreadIndicesType = vtkm::exec::arg::ThreadIndicesReduceByKey;
  using ExecObjectType =
    vtkm::exec::internal::ReduceByKeyLookup<KeyPortalType, IdPortalType, IdComponentPortalType>;

  using ValueType = typename ExecObjectType::KeyType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType& keys) const
  {
    return keys.UniqueKeys.Get(indices.GetInputIndex());
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

#endif //vtk_m_exec_arg_FetchTagKeysIn_h
