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
#ifndef vtkm_m_worklet_OrientPointNormals_h
#define vtkm_m_worklet_OrientPointNormals_h

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

namespace vtkm
{
namespace worklet
{

///
/// Orients normals to point outside of the dataset. This requires a closed
/// manifold surface or else the behavior is undefined. This requires an
/// unstructured cellset as input.
///
class OrientPointNormals
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
                                  FieldInOut normals,
                                  WholeArrayIn ranges,
                                  FieldOut activePoints,
                                  FieldOut visitedPoints,
                                  FieldOut refPoints);
    using ExecutionSignature =
      _6(InputIndex pointId, _1 coord, _2 normal, _3 ranges, _4 activePoints, _5 visitedPoints);

    template <typename CoordT, typename NormalT, typename RangePortal>
    VTKM_EXEC vtkm::Id operator()(const vtkm::Id pointId,
                                  const vtkm::Vec<CoordT, 3>& point,
                                  vtkm::Vec<NormalT, 3>& normal,
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
          Align(normal, ref);
          isActive = true;
          isVisited = true;
          return pointId;
        }
        else if (val >= range.Max)
        {
          vtkm::Vec<NormalT, 3> ref{ static_cast<NormalT>(0) };
          ref[dim] = static_cast<NormalT>(1);
          Align(normal, ref);
          isActive = true;
          isVisited = true;
          return pointId;
        }
      }

      isActive = false;
      isVisited = false;
      return INVALID_ID;
    }
  };

  // Traverses the active points (via mask) and marks the connected cells as
  // active. Set the reference point for all adjacent cells to the current
  // point.
  class WorkletMarkActiveCells : public vtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cellSet,
                                  // InOut to preserve data on masked indices
                                  BitFieldInOut activeCells,
                                  BitFieldInOut visitedCells,
                                  FieldInOutPoint activePoints);
    using ExecutionSignature = _4(CellIndices cells, _2 activeCells, _3 visitedCells);

    using MaskType = vtkm::worklet::MaskIndices;

    // Mark all unvisited cells as active:
    template <typename CellListT, typename ActiveCellsT, typename VisitedCellsT>
    VTKM_EXEC bool operator()(const CellListT& cells,
                              ActiveCellsT& activeCells,
                              VisitedCellsT& visitedCells) const
    {
      for (vtkm::IdComponent c = 0; c < cells.GetNumberOfComponents(); ++c)
      {
        const vtkm::Id cellId = cells[c];
        const bool alreadyVisited = visitedCells.CompareAndSwapBitAtomic(cellId, true, false);

        if (!alreadyVisited)
        { // This thread is first to visit cell
          activeCells.SetBitAtomic(cellId, true);
        }
      }

      // Mark the current point as inactive:
      return false;
    }
  };

  // Traverses the active cells and mark the connected points as active,
  // propogating the reference pointId.
  class WorkletMarkActivePoints : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellSet,
                                  BitFieldInOut activePoints,
                                  BitFieldIn visitedPoints,
                                  WholeArrayInOut refPoints,
                                  FieldInOutCell activeCells);
    using ExecutionSignature = _5(PointIndices points,
                                  _2 activePoints,
                                  _3 visitedPoints,
                                  _4 refPoints);

    using MaskType = vtkm::worklet::MaskIndices;

    template <typename PointListT,
              typename ActivePointsT,
              typename VisitedPointsT,
              typename RefPointsT>
    VTKM_EXEC bool operator()(const PointListT& points,
                              ActivePointsT& activePoints,
                              VisitedPointsT& visitedPoints,
                              RefPointsT& refPoints) const
    {
      // Find any point in the cell that has already been visited, and take
      // its id as the reference for this cell.
      vtkm::Id refPtId = INVALID_ID;
      for (vtkm::IdComponent p = 0; p < points.GetNumberOfComponents(); ++p)
      {
        const vtkm::Id pointId = points[p];
        const bool alreadyVisited = visitedPoints.GetBit(pointId);
        if (alreadyVisited)
        {
          refPtId = pointId;
          break;
        }
      }

      // There must be one valid point in each cell:
      VTKM_ASSERT("Reference point not found." && refPtId != INVALID_ID);

      // Propogate the reference point to other cell members
      for (vtkm::IdComponent p = 0; p < points.GetNumberOfComponents(); ++p)
      {
        const vtkm::Id pointId = points[p];

        // Mark this point as active
        const bool alreadyVisited = visitedPoints.GetBit(pointId);
        if (!alreadyVisited)
        {
          const bool alreadyActive = activePoints.CompareAndSwapBitAtomic(pointId, true, false);
          if (!alreadyActive)
          { // If we're the first thread to mark point active, set ref point:
            refPoints.Set(pointId, refPtId);
          }
        }
      }

      // Mark current cell as inactive:
      return false;
    }
  };

  // For each point with a refPtId set, ensure that the associated normal is
  // in the same hemisphere as the reference normal.
  // This must be done in a separate step from MarkActivePoints since modifying
  // visitedPoints in that worklet would create race conditions.
  class WorkletProcessNormals : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn refIds,
                                  WholeArrayInOut normals,
                                  // InOut to preserve data on masked indices
                                  BitFieldInOut visitedPoints);
    using ExecutionSignature = void(InputIndex ptId, _1 refPtId, _2 normals, _3 visitedPoints);

    using MaskType = vtkm::worklet::MaskIndices;

    template <typename NormalsPortal, typename VisitedPointsT>
    VTKM_EXEC void operator()(const vtkm::Id ptId,
                              const vtkm::Id refPtId,
                              NormalsPortal& normals,
                              VisitedPointsT& visitedPoints) const
    {
      visitedPoints.SetBitAtomic(ptId, true);

      using Normal = typename NormalsPortal::ValueType;
      Normal normal = normals.Get(ptId);
      const Normal ref = normals.Get(refPtId);
      if (Align(normal, ref))
      {
        normals.Set(ptId, normal);
      }
    }
  };

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename PointNormalCompType,
            typename PointNormalStorageType>
  VTKM_CONT static void Run(
    const CellSetType& cells,
    const vtkm::cont::ArrayHandle<vtkm::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    vtkm::cont::ArrayHandle<vtkm::Vec<PointNormalCompType, 3>, PointNormalStorageType>&
      pointNormals)
  {
    using RangeType = vtkm::cont::ArrayHandle<vtkm::Range>;

    using MarkSourcePoints = vtkm::worklet::DispatcherMapField<WorkletMarkSourcePoints>;
    using MarkActiveCells = vtkm::worklet::DispatcherMapTopology<WorkletMarkActiveCells>;
    using MarkActivePoints = vtkm::worklet::DispatcherMapTopology<WorkletMarkActivePoints>;
    using ProcessNormals = vtkm::worklet::DispatcherMapField<WorkletProcessNormals>;

    const vtkm::Id numCells = cells.GetNumberOfCells();

    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf,
                   "OrientPointNormals worklet (%lld points, %lld cells)",
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

    // For each point, store a reference alignment point. Allocated by
    // MarkSourcePoints.
    vtkm::cont::ArrayHandle<vtkm::Id> refPoints;

    // 1) Compute range of coords.
    const RangeType ranges = vtkm::cont::ArrayRangeCompute(coords);

    // 2) Label source points for traversal (use those on a boundary).
    //    Correct the normals for these points by making them point towards the
    //    boundary.
    {
      MarkSourcePoints dispatcher;
      dispatcher.Invoke(coords, pointNormals, ranges, activePoints, visitedPoints, refPoints);
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

      // 4) Mark unvisited points in active cells, using ref point from cell.
      {
        vtkm::Id numActive = vtkm::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);
        (void)numActive;
        VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
                   "MarkActivePoints from " << numActive << " active cells.");
        MarkActivePoints dispatcher{ vtkm::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, activePointBits, visitedPointBits, refPoints, activeCells);
      }

      vtkm::Id numActivePoints =
        vtkm::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);

      if (numActivePoints == 0)
      { // Done!
        VTKM_LOG_S(vtkm::cont::LogLevel::Perf, "Iteration " << iter << ": Traversal complete.");
        break;
      }

      VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
                 "Iteration " << iter << ": Processing " << numActivePoints << " normals.");

      // 5) Correct normals for active points.
      {
        ProcessNormals dispatcher{ vtkm::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(refPoints, pointNormals, visitedPointBits);
      }
    }
  }
};
}
} // end namespace vtkm::worklet


#endif // vtkm_m_worklet_OrientPointNormals_h
