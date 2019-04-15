//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_FetchTagCellSetIn_h
#define vtk_m_exec_arg_FetchTagCellSetIn_h

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ThreadIndicesTopologyMap.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting topology information.
///
/// \c FetchTagCellSetIn is a tag used with the \c Fetch class to retrieve
/// values from a topology object.  This default parameter returns
/// the basis topology type, i.e. cell type in a \c WorkletCellMap.
///
struct FetchTagCellSetIn
{
};

template <typename ConnectivityType, typename ExecObjectType>
struct Fetch<vtkm::exec::arg::FetchTagCellSetIn,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>,
             ExecObjectType>
{
  using ThreadIndicesType = vtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

  using ValueType = typename ThreadIndicesType::CellShapeTag;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    return indices.GetCellShape();
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

#endif //vtk_m_exec_arg_FetchTagCellSetIn_h
