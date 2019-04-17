//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_FetchTagArrayDirectInOut_h
#define vtk_m_exec_arg_FetchTagArrayDirectInOut_h

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for in-place modifying array values with direct indexing.
///
/// \c FetchTagArrayDirectInOut is a tag used with the \c Fetch class to do
/// in-place modification of values in an array portal. The fetch uses direct
/// indexing, so the thread index given to \c Store is used as the index into
/// the array.
///
/// When using \c FetchTagArrayDirectInOut with a worklet invocation with a
/// scatter, it is a bit undefined how the in/out array should be indexed.
/// Should it be the size of the input arrays and written back there, or
/// should it be the size of the output arrays and pre-filled with the output.
/// The implementation indexes based on the output because it is safer. The
/// output will have a unique index for each worklet instance, so you don't
/// have to worry about writes stomping on each other (which they would
/// inevitably do if index as input).
///
struct FetchTagArrayDirectInOut
{
};

template <typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<vtkm::exec::arg::FetchTagArrayDirectInOut,
             vtkm::exec::arg::AspectTagDefault,
             ThreadIndicesType,
             ExecObjectType>
{
  using ValueType = typename ExecObjectType::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType& arrayPortal) const
  {
    return arrayPortal.Get(indices.GetOutputIndex());
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void Store(const ThreadIndicesType& indices,
             const ExecObjectType& arrayPortal,
             const ValueType& value) const
  {
    arrayPortal.Set(indices.GetOutputIndex(), value);
  }
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FetchTagArrayDirectInOut_h
