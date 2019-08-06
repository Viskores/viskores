//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_m_worklet_OrientPointAndCellNormals_h
#define vtkm_m_worklet_OrientPointAndCellNormals_h

#include <vtkm/Range.h>
#include <vtkm/Types.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleBitField.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayRangeCompute.h>
#include <vtkm/cont/BitField.h>
#include <vtkm/cont/Logging.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/MaskIndices.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace worklet
{

///
/// Orients normals to point outside of the dataset. This requires a closed
/// manifold surface or else the behavior is undefined. This requires an
/// unstructured cellset as input.
///
class OrientPointAndCellNormals
{
  static constexpr vtkm::Id INVALID_ID = -1;

  // Returns true if v1 and v2 are pointing in the same hemisphere.
  template <typename T>
  VTKM_EXEC static bool SameDirection(const vtkm::Vec<T, 3>& v1, const vtkm::Vec<T, 3>& v2)
  {
    return vtkm::Dot(v1, v2) >= 0;
  }

  // Ensure that the normal is pointing in the same hemisphere as ref.
  // Returns true if normal is modified.
  template <typename T>
  VTKM_EXEC static bool Align(vtkm::Vec<T, 3>& normal, const vtkm::Vec<T, 3>& ref)
  {
    if (!SameDirection(normal, ref))
    {
      normal = -normal;
      return true;
    }
    return false;
  }

public:
  // Locates starting points for BFS traversal of dataset by finding points
  // on the dataset boundaries. The normals for these points are corrected by
  // making them point outside of the dataset, and they are marked as both
  // active and visited.
  class WorkletMarkSourcePoints : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn coords,
                                  FieldInOut pointNormals,
                                  WholeArrayIn ranges,
                                  FieldOut activePoints,
                                  FieldOut visitedPoints);
    using ExecutionSignature =
      void(_1 coord, _2 pointNormal, _3 ranges, _4 activePoints, _5 visitedPoints);

    template <typename CoordT, typename NormalT, typename RangePortal>
    VTKM_EXEC void operator()(const vtkm::Vec<CoordT, 3>& point,
                              vtkm::Vec<NormalT, 3>& pointNormal,
                              const RangePortal& ranges,
                              bool& isActive,
                              bool& isVisited) const
    {
      for (vtkm::IdComponent dim = 0; dim < 3; ++dim)
      {
        const auto& range = ranges.Get(dim);
        const auto val = static_cast<decltype(range.Min)>(point[dim]);
        if (val <= range.Min)
        {
          vtkm::Vec<NormalT, 3> ref{ static_cast<NormalT>(0) };
          ref[dim] = static_cast<NormalT>(-1);
          Align(pointNormal, ref);
          isActive = true;
          isVisited = true;
          return;
        }
        else if (val >= range.Max)
        {
          vtkm::Vec<NormalT, 3> ref{ static_cast<NormalT>(0) };
          ref[dim] = static_cast<NormalT>(1);
          Align(pointNormal, ref);
          isActive = true;
          isVisited = true;
          return;
        }
      }

      isActive = false;
      isVisited = false;
    }
  };

  // Mark each incident cell as active and visited.
  // Marks the current point as inactive.
  class WorkletMarkActiveCells : public vtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cell,
                                  BitFieldInOut activeCells,
                                  BitFieldInOut visitedCells,
                                  FieldInOutPoint activePoints);
    using ExecutionSignature = _4(CellIndices cellIds, _2 activeCells, _3 visitedCells);

    using MaskType = vtkm::worklet::MaskIndices;

    template <typename CellList, typename ActiveCellsBitPortal, typename VisitedCellsBitPortal>
    VTKM_EXEC bool operator()(const CellList& cellIds,
                              ActiveCellsBitPortal& activeCells,
                              VisitedCellsBitPortal& visitedCells) const
    {
      const vtkm::IdComponent numCells = cellIds.GetNumberOfComponents();
      for (vtkm::IdComponent c = 0; c < numCells; ++c)
      {
        const vtkm::Id cellId = cellIds[c];
        if (!visitedCells.OrBitAtomic(cellId, true))
        { // This thread owns this cell.
          activeCells.SetBitAtomic(cellId, true);
        }
      }

      // Mark current point as inactive:
      return false;
    }
  };

  // Align the current cell's normals to an adjacent visited point's normal.
  class WorkletProcessCellNormals : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cells,
                                  WholeArrayIn pointNormals,
                                  WholeArrayInOut cellNormals,
                                  BitFieldIn visitedPoints);
    using ExecutionSignature = void(PointIndices pointIds,
                                    InputIndex cellId,
                                    _2 pointNormals,
                                    _3 cellNormals,
                                    _4 visitedPoints);

    using MaskType = vtkm::worklet::MaskIndices;

    template <typename PointList,
              typename PointNormalsPortal,
              typename CellNormalsPortal,
              typename VisitedPointsBitPortal>
    VTKM_EXEC void operator()(const PointList& pointIds,
                              const vtkm::Id cellId,
                              const PointNormalsPortal& pointNormals,
                              CellNormalsPortal& cellNormals,
                              const VisitedPointsBitPortal& visitedPoints) const
    {
      // Use the normal of a visited point as a reference:
      const vtkm::Id refPointId = [&]() -> vtkm::Id {
        const vtkm::IdComponent numPoints = pointIds.GetNumberOfComponents();
        for (vtkm::IdComponent p = 0; p < numPoints; ++p)
        {
          const vtkm::Id pointId = pointIds[p];
          if (visitedPoints.GetBit(pointId))
          {
            return pointId;
          }
        }

        return INVALID_ID;
      }();

      VTKM_ASSERT("No reference point found." && refPointId != INVALID_ID);

      const auto refNormal = pointNormals.Get(refPointId);
      auto normal = cellNormals.Get(cellId);
      if (Align(normal, refNormal))
      {
        cellNormals.Set(cellId, normal);
      }
    }
  };

  // Mark each incident point as active and visited.
  // Marks the current cell as inactive.
  class WorkletMarkActivePoints : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cell,
                                  BitFieldInOut activePoints,
                                  BitFieldInOut visitedPoints,
                                  FieldInOutCell activeCells);
    using ExecutionSignature = _4(PointIndices pointIds, _2 activePoint, _3 visitedPoint);

    using MaskType = vtkm::worklet::MaskIndices;

    template <typename PointList, typename ActivePointsBitPortal, typename VisitedPointsBitPortal>
    VTKM_EXEC bool operator()(const PointList& pointIds,
                              ActivePointsBitPortal& activePoints,
                              VisitedPointsBitPortal& visitedPoints) const
    {
      const vtkm::IdComponent numPoints = pointIds.GetNumberOfComponents();
      for (vtkm::IdComponent p = 0; p < numPoints; ++p)
      {
        const vtkm::Id pointId = pointIds[p];
        if (!visitedPoints.OrBitAtomic(pointId, true))
        { // This thread owns this point.
          activePoints.SetBitAtomic(pointId, true);
        }
      }

      // Mark current cell as inactive:
      return false;
    }
  };

  // Align the current point's normals to an adjacent visited cell's normal.
  class WorkletProcessPointNormals : public vtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cells,
                                  WholeArrayInOut pointNormals,
                                  WholeArrayIn cellNormals,
                                  BitFieldIn visitedCells);
    using ExecutionSignature = void(CellIndices cellIds,
                                    InputIndex pointId,
                                    _2 pointNormals,
                                    _3 cellNormals,
                                    _4 visitedCells);

    using MaskType = vtkm::worklet::MaskIndices;

    template <typename CellList,
              typename CellNormalsPortal,
              typename PointNormalsPortal,
              typename VisitedCellsBitPortal>
    VTKM_EXEC void operator()(const CellList& cellIds,
                              const vtkm::Id pointId,
                              PointNormalsPortal& pointNormals,
                              const CellNormalsPortal& cellNormals,
                              const VisitedCellsBitPortal& visitedCells) const
    {
      // Use the normal of a visited cell as a reference:
      const vtkm::Id refCellId = [&]() -> vtkm::Id {
        const vtkm::IdComponent numCells = cellIds.GetNumberOfComponents();
        for (vtkm::IdComponent c = 0; c < numCells; ++c)
        {
          const vtkm::Id cellId = cellIds[c];
          if (visitedCells.GetBit(cellId))
          {
            return cellId;
          }
        }

        return INVALID_ID;
      }();

      VTKM_ASSERT("No reference cell found." && refCellId != INVALID_ID);

      const auto refNormal = cellNormals.Get(refCellId);
      auto normal = pointNormals.Get(pointId);
      if (Align(normal, refNormal))
      {
        pointNormals.Set(pointId, normal);
      }
    }
  };

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename PointNormalCompType,
            typename PointNormalStorageType,
            typename CellNormalCompType,
            typename CellNormalStorageType>
  VTKM_CONT static void Run(
    const CellSetType& cells,
    const vtkm::cont::ArrayHandle<vtkm::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    vtkm::cont::ArrayHandle<vtkm::Vec<PointNormalCompType, 3>, PointNormalStorageType>&
      pointNormals,
    vtkm::cont::ArrayHandle<vtkm::Vec<CellNormalCompType, 3>, CellNormalStorageType>& cellNormals)
  {
    using RangeType = vtkm::cont::ArrayHandle<vtkm::Range>;

    using MarkSourcePoints = vtkm::worklet::DispatcherMapField<WorkletMarkSourcePoints>;
    using MarkActiveCells = vtkm::worklet::DispatcherMapTopology<WorkletMarkActiveCells>;
    using ProcessCellNormals = vtkm::worklet::DispatcherMapTopology<WorkletProcessCellNormals>;
    using MarkActivePoints = vtkm::worklet::DispatcherMapTopology<WorkletMarkActivePoints>;
    using ProcessPointNormals = vtkm::worklet::DispatcherMapTopology<WorkletProcessPointNormals>;

    const vtkm::Id numCells = cells.GetNumberOfCells();

    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf,
                   "OrientPointAndCellNormals worklet (%lld points, %lld cells)",
                   static_cast<vtkm::Int64>(coords.GetNumberOfValues()),
                   static_cast<vtkm::Int64>(numCells));

    // active = cells / point to be used in the next worklet invocation mask.
    vtkm::cont::BitField activePointBits; // Initialized by MarkSourcePoints
    auto activePoints = vtkm::cont::make_ArrayHandleBitField(activePointBits);

    vtkm::cont::BitField activeCellBits;
    vtkm::cont::Algorithm::Fill(activeCellBits, false, numCells);
    auto activeCells = vtkm::cont::make_ArrayHandleBitField(activeCellBits);

    // visited = cells / points that have been corrected.
    vtkm::cont::BitField visitedPointBits; // Initialized by MarkSourcePoints
    auto visitedPoints = vtkm::cont::make_ArrayHandleBitField(visitedPointBits);

    vtkm::cont::BitField visitedCellBits;
    vtkm::cont::Algorithm::Fill(visitedCellBits, false, numCells);
    auto visitedCells = vtkm::cont::make_ArrayHandleBitField(visitedCellBits);

    vtkm::cont::ArrayHandle<vtkm::Id> mask; // Allocated as needed

    // 1) Compute range of coords.
    const RangeType ranges = vtkm::cont::ArrayRangeCompute(coords);

    // 2) Locate points on a boundary and align their normal to point out of the
    //    dataset:
    {
      MarkSourcePoints dispatcher;
      dispatcher.Invoke(coords, pointNormals, ranges, activePoints, visitedPoints);
    }

    for (size_t iter = 1;; ++iter)
    {
      // 3) Mark unvisited cells adjacent to active points
      {
        vtkm::Id numActive = vtkm::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);
        (void)numActive;
        VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
                   "MarkActiveCells from " << numActive << " active points.");
        MarkActiveCells dispatcher{ vtkm::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, activeCellBits, visitedCellBits, activePoints);
      }

      vtkm::Id numActiveCells = vtkm::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);

      if (numActiveCells == 0)
      { // Done!
        VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
                   "Iteration " << iter << ": Traversal complete; no more cells");
        break;
      }

      VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
                 "Iteration " << iter << ": Processing " << numActiveCells << " cell normals.");

      // 4) Correct normals for active cells.
      {
        ProcessCellNormals dispatcher{ vtkm::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, pointNormals, cellNormals, visitedPointBits);
      }

      // 5) Mark unvisited points adjacent to active cells
      {
        vtkm::Id numActive = vtkm::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);
        (void)numActive;
        VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
                   "MarkActivePoints from " << numActive << " active cells.");
        MarkActivePoints dispatcher{ vtkm::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, activePointBits, visitedPointBits, activeCells);
      }

      vtkm::Id numActivePoints =
        vtkm::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);

      if (numActivePoints == 0)
      { // Done!
        VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
                   "Iteration " << iter << ": Traversal complete; no more points");
        break;
      }

      VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
                 "Iteration " << iter << ": Processing " << numActivePoints << " point normals.");

      // 4) Correct normals for active points.
      {
        ProcessPointNormals dispatcher{ vtkm::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, pointNormals, cellNormals, visitedCellBits);
      }
    }
  }
};
}
} // end namespace vtkm::worklet


#endif // vtkm_m_worklet_OrientPointAndCellNormals_h
