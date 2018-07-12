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
    cellId = -1;
    vtkm::Id nodeIndex = 0;
    FindCellState state = FindCellState::EnterNode;

    while ((cellId < 0) && !((nodeIndex == 0) && (state == FindCellState::AscendFromNode)))
    {
      switch (state)
      {
        case FindCellState::EnterNode:
          this->EnterNode(state, point, cellId, nodeIndex, parametric, worklet);
          break;
        case FindCellState::AscendFromNode:
          this->AscendFromNode(state, nodeIndex);
          break;
        case FindCellState::DescendLeftChild:
          this->DescendLeftChild(state, point, nodeIndex);
          break;
        case FindCellState::DescendRightChild:
          this->DescendRightChild(state, point, nodeIndex);
          break;
      }
    }
  }

private:
  enum struct FindCellState
  {
    EnterNode,
    AscendFromNode,
    DescendLeftChild,
    DescendRightChild
  };

  VTKM_EXEC
  void EnterNode(FindCellState& state,
                 const vtkm::Vec<vtkm::FloatDefault, 3>& point,
                 vtkm::Id& cellId,
                 vtkm::Id nodeIndex,
                 vtkm::Vec<vtkm::FloatDefault, 3>& parametric,
                 const vtkm::exec::FunctorBase& worklet) const
  {
    VTKM_ASSERT(state == FindCellState::EnterNode);

    const vtkm::cont::BoundingIntervalHierarchyNode& node = this->Nodes.Get(nodeIndex);

    if (node.ChildIndex < 0)
    {
      // In a leaf node. Look for a containing cell.
      cellId = this->FindInLeaf(point, parametric, node, worklet);
      state = FindCellState::AscendFromNode;
    }
    else
    {
      state = FindCellState::DescendLeftChild;
    }
  }

  VTKM_EXEC
  void AscendFromNode(FindCellState& state, vtkm::Id& nodeIndex) const
  {
    VTKM_ASSERT(state == FindCellState::AscendFromNode);

    vtkm::Id childNodeIndex = nodeIndex;
    const vtkm::cont::BoundingIntervalHierarchyNode& childNode = this->Nodes.Get(childNodeIndex);
    nodeIndex = childNode.ParentIndex;
    const vtkm::cont::BoundingIntervalHierarchyNode& parentNode = this->Nodes.Get(nodeIndex);

    if (parentNode.ChildIndex == childNodeIndex)
    {
      // Ascending from left child. Descend into the right child.
      state = FindCellState::DescendRightChild;
    }
    else
    {
      VTKM_ASSERT(parentNode.ChildIndex + 1 == childNodeIndex);
      // Ascending from right child. Ascend again. (Don't need to change state.)
    }
  }

  VTKM_EXEC
  void DescendLeftChild(FindCellState& state,
                        const vtkm::Vec<vtkm::FloatDefault, 3>& point,
                        vtkm::Id& nodeIndex) const
  {
    VTKM_ASSERT(state == FindCellState::DescendLeftChild);

    const vtkm::cont::BoundingIntervalHierarchyNode& node = this->Nodes.Get(nodeIndex);
    const vtkm::FloatDefault& coordinate = point[node.Dimension];
    if (coordinate <= node.Node.LMax)
    {
      // Left child does contain the point. Do the actual descent.
      nodeIndex = node.ChildIndex;
      state = FindCellState::EnterNode;
    }
    else
    {
      // Left child does not contain the point. Skip to the right child.
      state = FindCellState::DescendRightChild;
    }
  }

  VTKM_EXEC
  void DescendRightChild(FindCellState& state,
                         const vtkm::Vec<vtkm::FloatDefault, 3>& point,
                         vtkm::Id& nodeIndex) const
  {
    VTKM_ASSERT(state == FindCellState::DescendRightChild);

    const vtkm::cont::BoundingIntervalHierarchyNode& node = this->Nodes.Get(nodeIndex);
    const vtkm::FloatDefault& coordinate = point[node.Dimension];
    if (coordinate >= node.Node.RMin)
    {
      // Right child does contain the point. Do the actual descent.
      nodeIndex = node.ChildIndex + 1;
      state = FindCellState::EnterNode;
    }
    else
    {
      // Right child does not contain the point. Skip to ascent
      state = FindCellState::AscendFromNode;
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
