//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_BoundingIntervalHierarchyExec_h
#define vtk_m_cont_BoundingIntervalHierarchyExec_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/VecFromPortalPermute.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleVirtualCoordinates.h>
#include <vtkm/cont/BoundingIntervalHierarchyNode.h>
#include <vtkm/exec/CellInside.h>
#include <vtkm/exec/CellLocator.h>
#include <vtkm/exec/ParametricCoordinates.h>

namespace vtkm
{
namespace exec
{
namespace
{
using NodeArrayHandle = vtkm::cont::ArrayHandle<vtkm::cont::BoundingIntervalHierarchyNode>;
using CellIdArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id>;

} // namespace

template <typename DeviceAdapter, typename CellSetType>
class BoundingIntervalHierarchyExec : public vtkm::exec::CellLocator
{
public:
  VTKM_CONT
  BoundingIntervalHierarchyExec() {}

  VTKM_CONT
  BoundingIntervalHierarchyExec(const NodeArrayHandle& nodes,
                                const CellIdArrayHandle& cellIds,
                                const CellSetType& cellSet,
                                const vtkm::cont::ArrayHandleVirtualCoordinates& coords,
                                DeviceAdapter)
    : Nodes(nodes.PrepareForInput(DeviceAdapter()))
    , CellIds(cellIds.PrepareForInput(DeviceAdapter()))
  {
    CellSet = cellSet.PrepareForInput(DeviceAdapter(), FromType(), ToType());
    Coords = coords.PrepareForInput(DeviceAdapter());
  }

  VTKM_EXEC
  void FindCell(const vtkm::Vec<vtkm::FloatDefault, 3>& point,
                vtkm::Id& cellId,
                vtkm::Vec<vtkm::FloatDefault, 3>& parametric,
                const vtkm::exec::FunctorBase& worklet) const override
  {
    cellId = Find(0, point, parametric, worklet);
  }

private:
  VTKM_EXEC vtkm::Id Find(vtkm::Id index,
                          const vtkm::Vec<vtkm::FloatDefault, 3>& point,
                          vtkm::Vec<vtkm::FloatDefault, 3>& parametric,
                          const vtkm::exec::FunctorBase& worklet) const
  {
    const vtkm::cont::BoundingIntervalHierarchyNode& node = Nodes.Get(index);
    if (node.ChildIndex < 0)
    {
      return FindInLeaf(point, parametric, node, worklet);
    }
    else
    {
      const vtkm::FloatDefault& c = point[node.Dimension];
      vtkm::Id id1 = -1;
      vtkm::Id id2 = -1;
      if (c <= node.Node.LMax)
      {
        id1 = Find(node.ChildIndex, point, parametric, worklet);
      }
      if (id1 == -1 && c >= node.Node.RMin)
      {
        id2 = Find(node.ChildIndex + 1, point, parametric, worklet);
      }
      if (id1 == -1 && id2 == -1)
      {
        return -1;
      }
      else if (id1 == -1)
      {
        return id2;
      }
      else
      {
        return id1;
      }
    }
  }

  VTKM_EXEC vtkm::Id FindInLeaf(const vtkm::Vec<vtkm::FloatDefault, 3>& point,
                                vtkm::Vec<vtkm::FloatDefault, 3>& parametric,
                                const vtkm::cont::BoundingIntervalHierarchyNode& node,
                                const vtkm::exec::FunctorBase& worklet) const
  {
    using IndicesType = typename CellSetPortal::IndicesType;
    for (vtkm::Id i = node.Leaf.Start; i < node.Leaf.Start + node.Leaf.Size; ++i)
    {
      vtkm::Id cellId = CellIds.Get(i);
      IndicesType cellPointIndices = CellSet.GetIndices(cellId);
      vtkm::VecFromPortalPermute<IndicesType, CoordsPortal> cellPoints(&cellPointIndices, Coords);
      if (IsPointInCell(point, parametric, CellSet.GetCellShape(cellId), cellPoints, worklet))
      {
        return cellId;
      }
    }
    return -1;
  }

  template <typename CoordsType, typename CellShapeTag>
  VTKM_EXEC static bool IsPointInCell(const vtkm::Vec<vtkm::FloatDefault, 3>& point,
                                      vtkm::Vec<vtkm::FloatDefault, 3>& parametric,
                                      CellShapeTag cellShape,
                                      const CoordsType& cellPoints,
                                      const vtkm::exec::FunctorBase& worklet)
  {
    bool success = false;
    parametric = vtkm::exec::WorldCoordinatesToParametricCoordinates(
      cellPoints, point, cellShape, success, worklet);
    return success && vtkm::exec::CellInside(parametric, cellShape);
  }

  using FromType = vtkm::TopologyElementTagPoint;
  using ToType = vtkm::TopologyElementTagCell;
  using NodePortal = typename NodeArrayHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using CellIdPortal =
    typename CellIdArrayHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using CellSetPortal =
    typename CellSetType::template ExecutionTypes<DeviceAdapter, FromType, ToType>::ExecObjectType;
  using CoordsPortal = typename vtkm::cont::ArrayHandleVirtualCoordinates::template ExecutionTypes<
    DeviceAdapter>::PortalConst;

  NodePortal Nodes;
  CellIdPortal CellIds;
  CellSetPortal CellSet;
  CoordsPortal Coords;
}; // class BoundingIntervalHierarchyExec

} // namespace exec

} // namespace vtkm

#endif //vtk_m_cont_BoundingIntervalHierarchyExec_h
