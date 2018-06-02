//============================================================================
////  Copyright (c) Kitware, Inc.
////  All rights reserved.
////  See LICENSE.txt for details.
////  This software is distributed WITHOUT ANY WARRANTY; without even
////  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
////  PURPOSE.  See the above copyright notice for more information.
////
////  Copyright 2015 Sandia Corporation.
////  Copyright 2015 UT-Battelle, LLC.
////  Copyright 2015 Los Alamos National Security.
////
////  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
////  the U.S. Government retains certain rights in this software.
////
////  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
////  Laboratory (LANL), the U.S. Government retains certain rights in
////  this software.
////============================================================================
#ifndef vtk_m_worklet_spatialstructure_BoundingIntervalHierarchy_h
#define vtk_m_worklet_spatialstructure_BoundingIntervalHierarchy_h

#include <vtkm/VecFromPortalPermute.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/exec/CellInside.h>
#include <vtkm/exec/ExecutionObjectBase.h>
#include <vtkm/exec/ParametricCoordinates.h>
#include <vtkm/worklet/spatialstructure/BoundingIntervalHierarchyNode.h>

namespace vtkm
{
namespace worklet
{
namespace spatialstructure
{
namespace
{

using NodeArrayHandle = vtkm::cont::ArrayHandle<BoundingIntervalHierarchyNode>;
using CellIdArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id>;

} // namespace

template <typename DeviceAdapter>
class BoundingIntervalHierarchyExecutionObject : public vtkm::exec::ExecutionObjectBase
{
public:
  VTKM_CONT
  BoundingIntervalHierarchyExecutionObject() {}

  VTKM_CONT
  BoundingIntervalHierarchyExecutionObject(const NodeArrayHandle& nodes,
                                           const CellIdArrayHandle& cellIds,
                                           DeviceAdapter)
    : Nodes(nodes.PrepareForInput(DeviceAdapter()))
    , CellIds(cellIds.PrepareForInput(DeviceAdapter()))
  {
  }

  template <typename CellSetType, typename PointPortal>
  VTKM_EXEC vtkm::Id Find(const vtkm::Vec<vtkm::Float64, 3>& point,
                          const CellSetType& cellSet,
                          const PointPortal& points,
                          const vtkm::exec::FunctorBase& worklet) const
  {
    return Find(0, point, cellSet, points, worklet);
  }

private:
  template <typename CellSetType, typename PointPortal>
  VTKM_EXEC vtkm::Id Find(vtkm::Id index,
                          const vtkm::Vec<vtkm::Float64, 3>& point,
                          const CellSetType& cellSet,
                          const PointPortal& points,
                          const vtkm::exec::FunctorBase& worklet) const
  {
    const BoundingIntervalHierarchyNode& node = Nodes.Get(index);
    if (node.ChildIndex < 0)
    {
      return FindInLeaf(point, node, cellSet, points, worklet);
    }
    else
    {
      const vtkm::Float64& c = point[node.Dimension];
      vtkm::Id id1 = -1;
      vtkm::Id id2 = -1;
      if (c <= node.Node.LMax)
      {
        id1 = Find(node.ChildIndex, point, cellSet, points, worklet);
      }
      if (id1 == -1 && c >= node.Node.RMin)
      {
        id2 = Find(node.ChildIndex + 1, point, cellSet, points, worklet);
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

  template <typename CellSetType, typename PointPortal>
  VTKM_EXEC vtkm::Id FindInLeaf(const vtkm::Vec<vtkm::Float64, 3>& point,
                                const BoundingIntervalHierarchyNode& node,
                                const CellSetType& cellSet,
                                const PointPortal& points,
                                const vtkm::exec::FunctorBase& worklet) const
  {
    using IndicesType = typename CellSetType::IndicesType;
    for (vtkm::Id i = node.Leaf.Start; i < node.Leaf.Start + node.Leaf.Size; ++i)
    {
      vtkm::Id cellId = CellIds.Get(i);
      IndicesType cellPointIndices = cellSet.GetIndices(cellId);
      vtkm::VecFromPortalPermute<IndicesType, PointPortal> cellPoints(&cellPointIndices, points);
      if (IsPointInCell(point, cellSet.GetCellShape(cellId), cellPoints, worklet))
      {
        return cellId;
      }
    }
    return -1;
  }

  template <typename CoordsType, typename CellShapeTag>
  VTKM_EXEC static bool IsPointInCell(const vtkm::Vec<vtkm::Float64, 3>& point,
                                      CellShapeTag cellShape,
                                      const CoordsType& cellPoints,
                                      const vtkm::exec::FunctorBase& worklet)
  {
    bool success = false;
    vtkm::Vec<vtkm::Float64, 3> parametricCoords =
      vtkm::exec::WorldCoordinatesToParametricCoordinates(
        cellPoints, point, cellShape, success, worklet);
    return success && vtkm::exec::CellInside(parametricCoords, cellShape);
  }

  using NodePortal = typename NodeArrayHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using CellIdPortal =
    typename CellIdArrayHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;

  NodePortal Nodes;
  CellIdPortal CellIds;
}; // class BoundingIntervalHierarchyExecutionObject

class BoundingIntervalHierarchy
{
public:
  VTKM_CONT
  BoundingIntervalHierarchy(const NodeArrayHandle& nodes, const CellIdArrayHandle& cellIds)
    : Nodes(nodes)
    , CellIds(cellIds)
  {
  }

  template <typename DeviceAdapter>
  VTKM_CONT BoundingIntervalHierarchyExecutionObject<DeviceAdapter> PrepareForInput()
  {
    return BoundingIntervalHierarchyExecutionObject<DeviceAdapter>(Nodes, CellIds, DeviceAdapter());
  }

private:
  NodeArrayHandle Nodes;
  CellIdArrayHandle CellIds;
}; // class BoundingIntervalHierarchy
}
}
} // namespace vtkm::worklet::spatialstructure

#endif //vtk_m_worklet_spatialstructure_BoundingIntervalHierarchy_h
