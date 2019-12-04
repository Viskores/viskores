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
#ifndef vtkm_m_worklet_OrientCellNormals_h
#define vtkm_m_worklet_OrientCellNormals_h

#include <vtkm/Range.h>
#include <vtkm/Types.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleBitField.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayRangeCompute.h>
#include <vtkm/cont/BitField.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/Logging.h>

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
class OrientCellNormals
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
  // on the dataset boundaries. These points are marked as active.
  // Initializes the ActivePoints array.
  class WorkletMarkSourcePoints : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn coords, WholeArrayIn ranges, FieldOut activePoints);
    using ExecutionSignature = _3(_1 coord, _2 ranges);

    template <typename CoordT, typename RangePortal>
    VTKM_EXEC bool operator()(const vtkm::Vec<CoordT, 3>& point, const RangePortal& ranges) const
    {
      bool isActive = false;
      for (vtkm::IdComponent dim = 0; dim < 3; ++dim)
      {
        const auto& range = ranges.Get(dim);
        const auto val = static_cast<decltype(range.Min)>(point[dim]);
        if (val <= range.Min || val >= range.Max)
        {
          isActive = true;
        }
      }
      return isActive;
    }
  };

  // For each of the source points, determine the boundaries it lies on. Align
  // each incident cell's normal to point out of the boundary, marking each cell
  // as both visited and active.
  // Clears the active flags for points, and marks the current point as visited.
  class WorkletProcessSourceCells : public vtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cells,
                                  FieldInPoint coords,
                                  WholeArrayIn ranges,
                                  WholeArrayInOut cellNormals,
                                  // InOut for preserve data on masked indices
                                  BitFieldInOut activeCells,
                                  BitFieldInOut visitedCells,
                                  FieldInOutPoint activePoints,
                                  FieldInOutPoint visitedPoints);
    using ExecutionSignature = void(CellIndices cellIds,
                                    _2 coords,
                                    _3 ranges,
                                    _4 cellNormals,
                                    _5 activeCells,
                                    _6 visitedCells,
                                    _7 activePoints,
                                    _8 visitedPoints);

    using MaskType = vtkm::worklet::MaskIndices;

    template <typename CellList,
              typename CoordComp,
              typename RangePortal,
              typename CellNormalPortal,
              typename ActiveCellsBitPortal,
              typename VisitedCellsBitPortal>
    VTKM_EXEC void operator()(const CellList& cellIds,
                              const vtkm::Vec<CoordComp, 3>& coord,
                              const RangePortal& ranges,
                              CellNormalPortal& cellNormals,
                              ActiveCellsBitPortal& activeCells,
                              VisitedCellsBitPortal& visitedCells,
                              bool& pointIsActive,
                              bool& pointIsVisited) const
    {
      using NormalType = typename CellNormalPortal::ValueType;
      VTKM_STATIC_ASSERT_MSG(vtkm::VecTraits<NormalType>::NUM_COMPONENTS == 3,
                             "Cell Normals expected to have 3 components.");
      using NormalCompType = typename NormalType::ComponentType;

      // Find the vector that points out of the dataset from the current point:
      const NormalType refNormal = [&]() -> NormalType {
        NormalType normal{ NormalCompType{ 0 } };
        NormalCompType numNormals{ 0 };
        for (vtkm::IdComponent dim = 0; dim < 3; ++dim)
        {
          const auto range = ranges.Get(dim);
          const auto val = static_cast<decltype(range.Min)>(coord[dim]);
          if (val <= range.Min)
          {
            NormalType compNormal{ NormalCompType{ 0 } };
            compNormal[dim] = NormalCompType{ -1 };
            normal += compNormal;
            ++numNormals;
          }
          else if (val >= range.Max)
          {
            NormalType compNormal{ NormalCompType{ 0 } };
            compNormal[dim] = NormalCompType{ 1 };
            normal += compNormal;
            ++numNormals;
          }
        }

        VTKM_ASSERT("Source point is not on a boundary?" && numNormals > 0.5);
        return normal / numNormals;
      }();

      // Align all cell normals to the reference, marking them as active and
      // visited.
      const vtkm::IdComponent numCells = cellIds.GetNumberOfComponents();
      for (vtkm::IdComponent c = 0; c < numCells; ++c)
      {
        const vtkm::Id cellId = cellIds[c];

        if (!visitedCells.OrBitAtomic(cellId, true))
        { // This thread is the first to touch this cell.
          activeCells.SetBitAtomic(cellId, true);

          NormalType cellNormal = cellNormals.Get(cellId);
          if (Align(cellNormal, refNormal))
          {
            cellNormals.Set(cellId, cellNormal);
          }
        }
      }

      // Mark current point as inactive but visited:
      pointIsActive = false;
      pointIsVisited = true;
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
    using ExecutionSignature = _4(PointIndices pointIds, _2 activePoints, _3 visitedPoints);

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

  // Mark each incident cell as active, setting a visited neighbor
  // cell as its reference for alignment.
  // Marks the current point as inactive.
  class WorkletMarkActiveCells : public vtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cells,
                                  WholeArrayOut refCells,
                                  BitFieldInOut activeCells,
                                  BitFieldIn visitedCells,
                                  FieldInOutPoint activePoints);
    using ExecutionSignature = _5(CellIndices cellIds,
                                  _2 refCells,
                                  _3 activeCells,
                                  _4 visitedCells);

    using MaskType = vtkm::worklet::MaskIndices;

    template <typename CellList,
              typename RefCellPortal,
              typename ActiveCellBitPortal,
              typename VisitedCellBitPortal>
    VTKM_EXEC bool operator()(const CellList& cellIds,
                              RefCellPortal& refCells,
                              ActiveCellBitPortal& activeCells,
                              const VisitedCellBitPortal& visitedCells) const
    {
      // One of the cells must be marked visited already. Find it and use it as
      // an alignment reference for the others:
      const vtkm::IdComponent numCells = cellIds.GetNumberOfComponents();
      const vtkm::Id refCellId = [&]() -> vtkm::Id {
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

      for (vtkm::IdComponent c = 0; c < numCells; ++c)
      {
        const vtkm::Id cellId = cellIds[c];
        if (!visitedCells.GetBit(cellId))
        {
          if (!activeCells.OrBitAtomic(cellId, true))
          { // This thread owns this cell.
            refCells.Set(cellId, refCellId);
          }
        }
      }

      // Mark current point as inactive:
      return false;
    }
  };

  // Align the normal of each active cell, to its reference cell normal.
  // The cell is marked visited.
  class WorkletProcessCellNormals : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn refCells,
                                  WholeArrayInOut cellNormals,
                                  FieldInOut visitedCells);
    using ExecutionSignature = _3(InputIndex cellId, _1 refCellId, _2 cellNormals);

    using MaskType = vtkm::worklet::MaskIndices;

    template <typename CellNormalsPortal>
    VTKM_EXEC bool operator()(const vtkm::Id cellId,
                              const vtkm::Id refCellId,
                              CellNormalsPortal& cellNormals) const
    {
      const auto refNormal = cellNormals.Get(refCellId);
      auto normal = cellNormals.Get(cellId);
      if (Align(normal, refNormal))
      {
        cellNormals.Set(cellId, normal);
      }

      // Mark cell as visited:
      return true;
    }
  };

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename CellNormalCompType,
            typename CellNormalStorageType>
  VTKM_CONT static void Run(
    const CellSetType& cells,
    const vtkm::cont::ArrayHandle<vtkm::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    vtkm::cont::ArrayHandle<vtkm::Vec<CellNormalCompType, 3>, CellNormalStorageType>& cellNormals)
  {
    using RangeType = vtkm::cont::ArrayHandle<vtkm::Range>;

    const vtkm::Id numPoints = coords.GetNumberOfValues();
    const vtkm::Id numCells = cells.GetNumberOfCells();

    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf,
                   "OrientCellNormals worklet (%lld points, %lld cells)",
                   static_cast<vtkm::Int64>(numPoints),
                   static_cast<vtkm::Int64>(numCells));

    // active = cells / point to be used in the next worklet invocation mask.
    vtkm::cont::BitField activePointBits; // Initialized by MarkSourcePoints
    auto activePoints = vtkm::cont::make_ArrayHandleBitField(activePointBits);

    vtkm::cont::BitField activeCellBits;
    vtkm::cont::Algorithm::Fill(activeCellBits, false, numCells);
    auto activeCells = vtkm::cont::make_ArrayHandleBitField(activeCellBits);

    // visited = cells / points that have been corrected.
    vtkm::cont::BitField visitedPointBits;
    vtkm::cont::Algorithm::Fill(visitedPointBits, false, numPoints);
    auto visitedPoints = vtkm::cont::make_ArrayHandleBitField(visitedPointBits);

    vtkm::cont::BitField visitedCellBits;
    vtkm::cont::Algorithm::Fill(visitedCellBits, false, numCells);
    auto visitedCells = vtkm::cont::make_ArrayHandleBitField(visitedCellBits);

    vtkm::cont::Invoker invoke;
    vtkm::cont::ArrayHandle<vtkm::Id> mask; // Allocated as needed

    // For each cell, store a reference alignment cell.
    vtkm::cont::ArrayHandle<vtkm::Id> refCells;
    {
      vtkm::cont::Algorithm::Copy(
        vtkm::cont::make_ArrayHandleConstant<vtkm::Id>(INVALID_ID, numCells), refCells);
    }

    // 1) Compute range of coords.
    const RangeType ranges = vtkm::cont::ArrayRangeCompute(coords);

    // 2) Locate points on a boundary, since their normal alignment direction
    //    is known.
    invoke(WorkletMarkSourcePoints{}, coords, ranges, activePoints);

    // 3) For each source point, align the normals of the adjacent cells.
    {
      vtkm::Id numActive = vtkm::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);
      (void)numActive;
      VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
                 "ProcessSourceCells from " << numActive << " source points.");
      invoke(WorkletProcessSourceCells{},
             vtkm::worklet::MaskIndices{ mask },
             cells,
             coords,
             ranges,
             cellNormals,
             activeCellBits,
             visitedCellBits,
             activePoints,
             visitedPoints);
    }

    for (size_t iter = 1;; ++iter)
    {
      // 4) Mark unvisited points adjacent to active cells
      {
        vtkm::Id numActive = vtkm::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);
        (void)numActive;
        VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
                   "MarkActivePoints from " << numActive << " active cells.");
        invoke(WorkletMarkActivePoints{},
               vtkm::worklet::MaskIndices{ mask },
               cells,
               activePointBits,
               visitedPointBits,
               activeCells);
      }

      // 5) Mark unvisited cells adjacent to active points
      {
        vtkm::Id numActive = vtkm::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);
        (void)numActive;
        VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
                   "MarkActiveCells from " << numActive << " active points.");
        invoke(WorkletMarkActiveCells{},
               vtkm::worklet::MaskIndices{ mask },
               cells,
               refCells,
               activeCellBits,
               visitedCellBits,
               activePoints);
      }

      vtkm::Id numActiveCells = vtkm::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);

      if (numActiveCells == 0)
      { // Done!
        VTKM_LOG_S(vtkm::cont::LogLevel::Perf, "Iteration " << iter << ": Traversal complete.");
        break;
      }

      VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
                 "Iteration " << iter << ": Processing " << numActiveCells << " normals.");

      // 5) Correct normals for active cells.
      {
        invoke(WorkletProcessCellNormals{},
               vtkm::worklet::MaskIndices{ mask },
               refCells,
               cellNormals,
               visitedCells);
      }
    }
  }
};
}
} // end namespace vtkm::worklet


#endif // vtkm_m_worklet_OrientCellNormals_h
