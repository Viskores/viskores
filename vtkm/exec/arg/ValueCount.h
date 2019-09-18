//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_ValueCount_h
#define vtk_m_exec_arg_ValueCount_h

#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ThreadIndicesReduceByKey.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the value count.
///
/// The \c AspectTagValueCount aspect tag causes the \c Fetch class to obtain
/// the number of values that map to the key.
///
struct AspectTagValueCount
{
};

/// \brief The \c ExecutionSignature tag to get the number of values.
///
/// A \c WorkletReduceByKey operates by collecting all values associated with
/// identical keys and then giving the worklet a Vec-like object containing all
/// values with a matching key. This \c ExecutionSignature tag provides the
/// number of values associated with the key and given in the Vec-like objects.
///
struct ValueCount : vtkm::exec::arg::ExecutionSignatureTagBase
{
  static constexpr vtkm::IdComponent INDEX = 1;
  using AspectTag = vtkm::exec::arg::AspectTagValueCount;
};

template <typename FetchTag, typename ExecObjectType>
struct Fetch<FetchTag,
             vtkm::exec::arg::AspectTagValueCount,
             vtkm::exec::arg::ThreadIndicesReduceByKey,
             ExecObjectType>
{
  using ThreadIndicesType = vtkm::exec::arg::ThreadIndicesReduceByKey;

  using ValueType = vtkm::IdComponent;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetNumberOfValues();
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

#endif //vtk_m_exec_arg_ValueCount_h
