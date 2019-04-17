//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_FetchTagExecObject_h
#define vtk_m_exec_arg_FetchTagExecObject_h

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>

#include <type_traits>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for execution objects.
///
/// \c FetchTagExecObject is a tag used with the \c Fetch class to retrieve
/// execution objects. For safety, execution objects are read-only. \c
/// FetchTagExecObject is almost always used in conjunction with \c
/// TransportTagExecObject and vice versa.
///
struct FetchTagExecObject
{
};

template <typename ThreadIndicesType, typename ExecObjectType>
struct Fetch<vtkm::exec::arg::FetchTagExecObject,
             vtkm::exec::arg::AspectTagDefault,
             ThreadIndicesType,
             ExecObjectType>
{
  using ValueType = ExecObjectType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& vtkmNotUsed(indices),
                 const ExecObjectType& execObject) const
  {
    return execObject;
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

#endif //vtk_m_exec_arg_FetchTagExecObject_h
