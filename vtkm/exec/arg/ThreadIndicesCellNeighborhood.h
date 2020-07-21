//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_ThreadIndicesCellNeighborhood_h
#define vtk_m_exec_arg_ThreadIndicesCellNeighborhood_h

#include <vtkm/exec/BoundaryState.h>
#include <vtkm/exec/ConnectivityStructured.h>
#include <vtkm/exec/arg/ThreadIndicesBasic.h>
#include <vtkm/exec/arg/ThreadIndicesNeighborhood.h>
#include <vtkm/exec/arg/ThreadIndicesTopologyMap.h> //for Deflate and Inflate

#include <vtkm/Math.h>

namespace vtkm
{
namespace exec
{
namespace arg
{
/// \brief Container for thread information in a WorkletCellNeighborhood.
///
///
class ThreadIndicesCellNeighborhood : public vtkm::exec::arg::ThreadIndicesNeighborhood
{
  using Superclass = vtkm::exec::arg::ThreadIndicesNeighborhood;

public:
  template <vtkm::IdComponent Dimension>
  VTKM_EXEC ThreadIndicesCellNeighborhood(
    const vtkm::Id3& threadIndex3D,
    vtkm::Id threadIndex1D,
    const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                             vtkm::TopologyElementTagCell,
                                             Dimension>& connectivity)
    : Superclass(
        threadIndex1D,
        vtkm::exec::BoundaryState{ threadIndex3D, detail::To3D(connectivity.GetCellDimensions()) })
  {
  }

  template <vtkm::IdComponent Dimension>
  VTKM_EXEC ThreadIndicesCellNeighborhood(
    const vtkm::Id3& threadIndex3D,
    vtkm::Id threadIndex1D,
    vtkm::Id inputIndex,
    vtkm::IdComponent visitIndex,
    vtkm::Id outputIndex,
    const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                             vtkm::TopologyElementTagCell,
                                             Dimension>& connectivity)
    : Superclass(
        threadIndex1D,
        inputIndex,
        visitIndex,
        outputIndex,
        vtkm::exec::BoundaryState{ threadIndex3D, detail::To3D(connectivity.GetCellDimensions()) })
  {
  }

  template <vtkm::IdComponent Dimension>
  VTKM_EXEC ThreadIndicesCellNeighborhood(
    vtkm::Id threadIndex,
    vtkm::Id inputIndex,
    vtkm::IdComponent visitIndex,
    vtkm::Id outputIndex,
    const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                             vtkm::TopologyElementTagCell,
                                             Dimension>& connectivity)
    : Superclass(
        threadIndex,
        inputIndex,
        visitIndex,
        outputIndex,
        vtkm::exec::BoundaryState{ detail::To3D(connectivity.FlatToLogicalToIndex(inputIndex)),
                                   detail::To3D(connectivity.GetCellDimensions()) })
  {
  }
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_ThreadIndicesCellNeighborhood_h
