//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_CellShape_h
#define vtk_m_exec_arg_CellShape_h

#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <vtkm/exec/arg/Fetch.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief Aspect tag to use for getting the cell shape.
///
/// The \c AspectTagCellShape aspect tag causes the \c Fetch class to
/// obtain the type of element (e.g. cell cell) from the topology object.
///
struct AspectTagCellShape
{
};

/// \brief The \c ExecutionSignature tag to use to get the cell shape.
///
struct CellShape : vtkm::exec::arg::ExecutionSignatureTagBase
{
  static constexpr vtkm::IdComponent INDEX = 1;
  using AspectTag = vtkm::exec::arg::AspectTagCellShape;
};

template <typename FetchTag, typename ExecObjectType>
struct Fetch<FetchTag, vtkm::exec::arg::AspectTagCellShape, ExecObjectType>
{
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VTKM_EXEC auto Load(const ThreadIndicesType& indices, const ExecObjectType&) const
    -> decltype(indices.GetCellShape())
  {
    return indices.GetCellShape();
  }

  template <typename ThreadIndicesType, typename ValueType>
  VTKM_EXEC void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op.
  }
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_CellShape_h
