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

#include <type_traits>

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
  using ValueType = typename ExecObjectType::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VTKM_EXEC ValueType Load(const ThreadIndicesType& indices,
                           const ExecObjectType& arrayPortal) const
  {
    return this->DoLoad(
      indices, arrayPortal, typename std::is_default_constructible<ValueType>::type{});
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType, typename T>
  VTKM_EXEC void Store(const ThreadIndicesType& indices,
                       const ExecObjectType& arrayPortal,
                       const T& value) const
  {
    arrayPortal.Set(indices.GetOutputIndex(), static_cast<ValueType>(value));
  }

private:
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VTKM_EXEC ValueType DoLoad(const ThreadIndicesType&, const ExecObjectType&, std::true_type) const
  {
    // Load is a no-op for this fetch.
    return ValueType();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VTKM_EXEC ValueType DoLoad(const ThreadIndicesType& indices,
                             const ExecObjectType& arrayPortal,
                             std::false_type) const
  {
    // Cannot create a ValueType object, so pull one out of the array portal. This may seem
    // weird because an output array often has garbage in it. However, this case can happen
    // with special arrays with Vec-like values that reference back to the array memory.
    // For example, with ArrayHandleRecombineVec, the values are actual objects that point
    // back to the array for on demand reading and writing. You need the buffer established
    // by the array even if there is garbage in that array.
    return arrayPortal.Get(indices.GetOutputIndex());
  }
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FetchTagArrayDirectOut_h
