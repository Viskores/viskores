//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_exec_CellLocatorTwoLevel_h
#define viskores_exec_CellLocatorTwoLevel_h

#include <viskores/exec/internal/CellLocatorHelpers.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/internal/CellLocatorCoordinates.h>

#include <viskores/Math.h>
#include <viskores/TopologyElementTag.h>
#include <viskores/Types.h>
#include <viskores/VecFromPortalPermute.h>
#include <viskores/VecTraits.h>

namespace viskores
{
namespace internal
{
namespace cl_uniform_bins
{

using DimensionType = viskores::Int16;
using DimVec3 = viskores::Vec<DimensionType, 3>;
using FloatVec3 = viskores::Vec3f;

struct Grid
{
  DimVec3 Dimensions;
  // Bug in CUDA 9.2 where having this gap for alignment was for some reason setting garbage
  // in a union with other cell locators (or perhaps not properly copying data). This appears
  // to be fixed by CUDA 10.2.
  DimensionType Padding;
  FloatVec3 Origin;
  FloatVec3 BinSize;
};

struct Bounds
{
  FloatVec3 Min;
  FloatVec3 Max;
};

VISKORES_EXEC inline viskores::Id ComputeFlatIndex(const DimVec3& idx, const DimVec3 dim)
{
  return idx[0] + (dim[0] * (idx[1] + (dim[1] * idx[2])));
}

VISKORES_EXEC inline Grid ComputeLeafGrid(const DimVec3& idx,
                                          const DimVec3& dim,
                                          const Grid& l1Grid)
{
  return { dim,
           0,
           l1Grid.Origin + (static_cast<FloatVec3>(idx) * l1Grid.BinSize),
           l1Grid.BinSize / static_cast<FloatVec3>(dim) };
}

template <typename PointsVecType>
VISKORES_EXEC inline Bounds ComputeCellBounds(const PointsVecType& points)
{
  using CoordsType = typename viskores::VecTraits<PointsVecType>::ComponentType;
  auto numPoints = viskores::VecTraits<PointsVecType>::GetNumberOfComponents(points);

  CoordsType minp = points[0], maxp = points[0];
  for (viskores::IdComponent i = 1; i < numPoints; ++i)
  {
    minp = viskores::Min(minp, points[i]);
    maxp = viskores::Max(maxp, points[i]);
  }

  return { FloatVec3(minp), FloatVec3(maxp) };
}
}
}
} // viskores::internal::cl_uniform_bins

namespace viskores
{
namespace exec
{

//--------------------------------------------------------------------

/// @brief Structure for locating cells.
///
/// Use the `FindCell()` method to identify which cell contains a point in space.
/// The `FindCell()` method optionally takes a `LastCell` object, which is a
/// structure nested in this class. The `LastCell` object can help speed locating
/// cells for successive finds at nearby points.
///
/// This class is provided by `viskores::cont::CellLocatorTwoLevel`
/// when passed to a worklet.
template <typename CellStructureType>
class VISKORES_ALWAYS_EXPORT CellLocatorTwoLevel
{
private:
  using DimVec3 = viskores::internal::cl_uniform_bins::DimVec3;
  using FloatVec3 = viskores::internal::cl_uniform_bins::FloatVec3;
  template <typename T>
  using ReadPortal = typename viskores::cont::ArrayHandle<T>::ReadPortalType;

public:
  template <typename CellSetType>
  VISKORES_CONT CellLocatorTwoLevel(const viskores::internal::cl_uniform_bins::Grid& topLevelGrid,
                                    const viskores::cont::ArrayHandle<DimVec3>& leafDimensions,
                                    const viskores::cont::ArrayHandle<viskores::Id>& leafStartIndex,
                                    const viskores::cont::ArrayHandle<viskores::Id>& cellStartIndex,
                                    const viskores::cont::ArrayHandle<viskores::Id>& cellCount,
                                    const viskores::cont::ArrayHandle<viskores::Id>& cellIds,
                                    const CellSetType& cellSet,
                                    const viskores::cont::CoordinateSystem& coords,
                                    viskores::cont::DeviceAdapterId device,
                                    viskores::cont::Token& token)
    : TopLevel(topLevelGrid)
    , LeafDimensions(leafDimensions.PrepareForInput(device, token))
    , LeafStartIndex(leafStartIndex.PrepareForInput(device, token))
    , CellStartIndex(cellStartIndex.PrepareForInput(device, token))
    , CellCount(cellCount.PrepareForInput(device, token))
    , CellIds(cellIds.PrepareForInput(device, token))
    , CellSet(cellSet.PrepareForInput(device,
                                      viskores::TopologyElementTagCell{},
                                      viskores::TopologyElementTagPoint{},
                                      token))
    , Coords(viskores::cont::internal::PrepareCellLocatorCoordinates(coords, device, token))
  {
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::LastCell
  struct LastCell
  {
    viskores::Id CellId = -1;
    viskores::Id LeafIdx = -1;
  };

  /// @brief Locate the id of the cell containing the provided point.
  ///
  /// This method returns the same cell id as `FindCell()` without requiring
  /// the caller to provide parametric coordinates.
  VISKORES_EXEC
  viskores::ErrorCode FindCellId(const FloatVec3& point, viskores::Id& cellId) const
  {
    LastCell lastCell;
    return this->Coords.CastAndCall(FindCellFunctor{}, this, point, cellId, lastCell, nullptr);
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindCellId
  VISKORES_EXEC
  viskores::ErrorCode FindCellId(const FloatVec3& point,
                                 viskores::Id& cellId,
                                 LastCell& lastCell) const
  {
    return this->Coords.CastAndCall(FindCellFunctor{}, this, point, cellId, lastCell, nullptr);
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindCell
  VISKORES_EXEC
  viskores::ErrorCode FindCell(const FloatVec3& point,
                               viskores::Id& cellId,
                               FloatVec3& pCoords) const
  {
    LastCell lastCell;
    return this->Coords.CastAndCall(FindCellFunctor{}, this, point, cellId, lastCell, &pCoords);
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindCell
  VISKORES_EXEC
  viskores::ErrorCode FindCell(const FloatVec3& point,
                               viskores::Id& cellId,
                               FloatVec3& pCoords,
                               LastCell& lastCell) const
  {
    return this->Coords.CastAndCall(FindCellFunctor{}, this, point, cellId, lastCell, &pCoords);
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::CountAllCells
  VISKORES_EXEC viskores::IdComponent CountAllCells(const viskores::Vec3f& point) const
  {
    return this->Coords.CastAndCall(CountAllCellsFunctor{}, this, point);
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindAllCells
  template <typename CellIdsType, typename ParametricCoordsVecType>
  VISKORES_EXEC viskores::ErrorCode FindAllCells(const viskores::Vec3f& point,
                                                 CellIdsType& cellIdVec,
                                                 ParametricCoordsVecType& pCoordsVec) const
  {
    return this->Coords.CastAndCall(FindAllCellsFunctor{}, this, point, cellIdVec, pCoordsVec);
  }

  /// @brief Locate all cell ids containing the provided point.
  ///
  /// This method returns the same cell ids as `FindAllCells()` without
  /// requiring parametric coordinates.
  template <typename CellIdsType>
  VISKORES_EXEC viskores::ErrorCode FindAllCellIds(const viskores::Vec3f& point,
                                                   CellIdsType& cellIdVec) const
  {
    return this->Coords.CastAndCall(FindAllCellIdsFunctor{}, this, point, cellIdVec);
  }

private:
  struct FindCellFunctor
  {
    template <typename CoordsPortalType>
    VISKORES_EXEC viskores::ErrorCode operator()(const CoordsPortalType& coords,
                                                 const CellLocatorTwoLevel* self,
                                                 const FloatVec3& point,
                                                 viskores::Id& cellId,
                                                 LastCell& lastCell,
                                                 FloatVec3* pCoords) const
    {
      return self->FindCellImpl(coords, point, cellId, lastCell, pCoords);
    }
  };

  struct CountAllCellsFunctor
  {
    template <typename CoordsPortalType>
    VISKORES_EXEC viskores::IdComponent operator()(const CoordsPortalType& coords,
                                                   const CellLocatorTwoLevel* self,
                                                   const FloatVec3& point) const
    {
      return self->CountAllCellsImpl(coords, point);
    }
  };

  struct FindAllCellsFunctor
  {
    template <typename CoordsPortalType, typename CellIdsType, typename ParametricCoordsVecType>
    VISKORES_EXEC viskores::ErrorCode operator()(const CoordsPortalType& coords,
                                                 const CellLocatorTwoLevel* self,
                                                 const FloatVec3& point,
                                                 CellIdsType& cellIdVec,
                                                 ParametricCoordsVecType& pCoordsVec) const
    {
      return self->FindAllCellsImpl(coords, point, cellIdVec, pCoordsVec);
    }
  };

  struct FindAllCellIdsFunctor
  {
    template <typename CoordsPortalType, typename CellIdsType>
    VISKORES_EXEC viskores::ErrorCode operator()(const CoordsPortalType& coords,
                                                 const CellLocatorTwoLevel* self,
                                                 const FloatVec3& point,
                                                 CellIdsType& cellIdVec) const
    {
      return self->FindAllCellsImpl(coords, point, cellIdVec);
    }
  };

  template <typename CoordsPortalType>
  VISKORES_EXEC viskores::ErrorCode FindCellImpl(const CoordsPortalType& coords,
                                                 const FloatVec3& point,
                                                 viskores::Id& cellId,
                                                 LastCell& lastCell,
                                                 FloatVec3* pCoords) const
  {
    viskores::Vec3f pc;
    if ((lastCell.CellId >= 0) && (lastCell.CellId < this->CellSet.GetNumberOfElements()) &&
        this->PointInCell(coords, point, lastCell.CellId, pc) == viskores::ErrorCode::Success)
    {
      cellId = lastCell.CellId;
      if (pCoords != nullptr)
        *pCoords = pc;
      return viskores::ErrorCode::Success;
    }

    if ((lastCell.LeafIdx >= 0) && (lastCell.LeafIdx < this->CellCount.GetNumberOfValues()) &&
        this->PointInLeaf(coords, point, lastCell.LeafIdx, cellId, pc) ==
          viskores::ErrorCode::Success)
    {
      lastCell.CellId = cellId;
      if (pCoords != nullptr)
        *pCoords = pc;
      return viskores::ErrorCode::Success;
    }

    viskores::Vec<viskores::Id, 1> cellIdVec = { -1 };
    viskores::IdComponent nCells = 0;
    if (pCoords != nullptr)
    {
      viskores::Vec<viskores::Vec3f, 1> pCoordsVec;
      nCells =
        this->IterateLeaves(coords, point, IterateMode::FindOne, cellIdVec, pCoordsVec, lastCell);
      if (nCells == 1)
        *pCoords = pCoordsVec[0];
    }
    else
    {
      nCells = this->IterateLeavesCellIds(coords, point, IterateMode::FindOne, cellIdVec, lastCell);
    }

    if (nCells == 0)
    {
      cellId = -1;
      return viskores::ErrorCode::CellNotFound;
    }

    cellId = cellIdVec[0];
    return viskores::ErrorCode::Success;
  }

  template <typename CoordsPortalType>
  VISKORES_EXEC viskores::IdComponent CountAllCellsImpl(const CoordsPortalType& coords,
                                                        const viskores::Vec3f& point) const
  {
    viskores::Vec<viskores::Id, 1> cellIdVec = { -1 };
    LastCell lastCell;

    return this->IterateLeavesCellIds(coords, point, IterateMode::CountAll, cellIdVec, lastCell);
  }

  template <typename CellIdsType>
  VISKORES_EXEC viskores::IdComponent InitializeAllCellIds(CellIdsType& cellIdVec) const
  {
    viskores::IdComponent n = cellIdVec.GetNumberOfComponents();
    for (viskores::IdComponent i = 0; i < n; i++)
      cellIdVec[i] = -1;
    return n;
  }

  template <typename CoordsPortalType, typename CellIdsType, typename ParametricCoordsVecType>
  VISKORES_EXEC viskores::ErrorCode FindAllCellsImpl(const CoordsPortalType& coords,
                                                     const viskores::Vec3f& point,
                                                     CellIdsType& cellIdVec,
                                                     ParametricCoordsVecType& pCoordsVec) const
  {
    VISKORES_ASSERT(pCoordsVec.GetNumberOfComponents() == cellIdVec.GetNumberOfComponents());
    auto writeHit =
      WriteCellAndParametric<CellIdsType, ParametricCoordsVecType>{ cellIdVec, pCoordsVec };
    return this->FindAllCellsImplCommon(coords, point, cellIdVec, writeHit);
  }

  template <typename CoordsPortalType, typename CellIdsType>
  VISKORES_EXEC viskores::ErrorCode FindAllCellsImpl(const CoordsPortalType& coords,
                                                     const viskores::Vec3f& point,
                                                     CellIdsType& cellIdVec) const
  {
    auto writeHit = WriteCellIdOnly<CellIdsType>{ cellIdVec };
    return this->FindAllCellsImplCommon(coords, point, cellIdVec, writeHit);
  }

  template <typename CoordsPortalType, typename CellIdsType, typename WriteHitType>
  VISKORES_EXEC viskores::ErrorCode FindAllCellsImplCommon(const CoordsPortalType& coords,
                                                           const viskores::Vec3f& point,
                                                           CellIdsType& cellIdVec,
                                                           WriteHitType& writeHit) const
  {
    viskores::IdComponent n = this->InitializeAllCellIds(cellIdVec);
    if (n == 0)
      return viskores::ErrorCode::Success;

    LastCell lastCell;
    viskores::IdComponent nCells =
      this->IterateLeavesImpl(coords, point, IterateMode::FindAll, cellIdVec, lastCell, writeHit);
    VISKORES_ASSERT(n == nCells);
    if (nCells == 0)
      return viskores::ErrorCode::CellNotFound;

    return viskores::ErrorCode::Success;
  }
  // Shared leaf traversal: modes are FindOne, CountAll, FindAll
  enum class IterateMode
  {
    FindOne,
    CountAll,
    FindAll
  };

  template <typename CellIdVecType, typename ParametricCoordsVecType>
  struct WriteCellAndParametric
  {
    CellIdVecType& CellIds;
    ParametricCoordsVecType& ParametricCoords;

    VISKORES_EXEC void operator()(viskores::IdComponent idx,
                                  viskores::Id cellId,
                                  const viskores::Vec3f& pc)
    {
      this->CellIds[idx] = cellId;
      this->ParametricCoords[idx] = pc;
    }
  };

  template <typename CellIdVecType>
  struct WriteCellIdOnly
  {
    CellIdVecType& CellIds;

    VISKORES_EXEC void operator()(viskores::IdComponent idx,
                                  viskores::Id cellId,
                                  const viskores::Vec3f&)
    {
      this->CellIds[idx] = cellId;
    }
  };

  template <typename CoordsPortalType, typename CellIdVecType, typename WriteHitType>
  VISKORES_EXEC viskores::IdComponent IterateLeavesImpl(const CoordsPortalType& coords,
                                                        const FloatVec3& point,
                                                        const IterateMode& mode,
                                                        CellIdVecType& cellIdVec,
                                                        LastCell& lastCell,
                                                        WriteHitType& writeHit) const
  {
    using namespace viskores::internal::cl_uniform_bins;

    viskores::IdComponent n = cellIdVec.GetNumberOfComponents();
    lastCell.CellId = -1;
    lastCell.LeafIdx = -1;
    viskores::IdComponent cellCnt = 0;

    DimVec3 binId3 = static_cast<DimVec3>((point - this->TopLevel.Origin) / this->TopLevel.BinSize);
    if (binId3[0] >= 0 && binId3[0] < this->TopLevel.Dimensions[0] && binId3[1] >= 0 &&
        binId3[1] < this->TopLevel.Dimensions[1] && binId3[2] >= 0 &&
        binId3[2] < this->TopLevel.Dimensions[2])
    {
      viskores::Id binId = ComputeFlatIndex(binId3, this->TopLevel.Dimensions);

      auto ldim = this->LeafDimensions.Get(binId);
      if (!ldim[0] || !ldim[1] || !ldim[2])
      {
        return 0;
      }

      auto leafGrid = ComputeLeafGrid(binId3, ldim, this->TopLevel);

      DimVec3 leafId3 = static_cast<DimVec3>((point - leafGrid.Origin) / leafGrid.BinSize);
      // precision issues may cause leafId3 to be out of range so clamp it
      leafId3 = viskores::Max(DimVec3(0), viskores::Min(ldim - DimVec3(1), leafId3));

      viskores::Id leafStart = this->LeafStartIndex.Get(binId);
      viskores::Id leafIdx = leafStart + ComputeFlatIndex(leafId3, leafGrid.Dimensions);

      viskores::Id start = this->CellStartIndex.Get(leafIdx);
      viskores::Id end = start + this->CellCount.Get(leafIdx);
      for (viskores::Id i = start; i < end; ++i)
      {
        viskores::Id cellId = this->CellIds.Get(i);
        viskores::Vec3f pc;
        if (this->PointInCell(coords, point, cellId, pc) == viskores::ErrorCode::Success)
        {
          lastCell.CellId = cellId;
          lastCell.LeafIdx = leafIdx;
          if (mode != IterateMode::CountAll)
            writeHit(cellCnt, cellId, pc);

          cellCnt++;
          if (mode == IterateMode::FindOne || (mode == IterateMode::FindAll && cellCnt == n))
            break;
        }
      }
    }

    return cellCnt;
  }

  template <typename CoordsPortalType, typename CellIdVecType, typename ParametricCoordsVecType>
  VISKORES_EXEC viskores::IdComponent IterateLeaves(const CoordsPortalType& coords,
                                                    const FloatVec3& point,
                                                    const IterateMode& mode,
                                                    CellIdVecType& cellIdVec,
                                                    ParametricCoordsVecType& pCoordsVec,
                                                    LastCell& lastCell) const
  {
    viskores::IdComponent n = cellIdVec.GetNumberOfComponents();
    VISKORES_ASSERT(pCoordsVec.GetNumberOfComponents() == n);
    auto writeHit =
      WriteCellAndParametric<CellIdVecType, ParametricCoordsVecType>{ cellIdVec, pCoordsVec };
    return this->IterateLeavesImpl(coords, point, mode, cellIdVec, lastCell, writeHit);
  }

  template <typename CoordsPortalType, typename CellIdVecType>
  VISKORES_EXEC viskores::IdComponent IterateLeavesCellIds(const CoordsPortalType& coords,
                                                           const FloatVec3& point,
                                                           const IterateMode& mode,
                                                           CellIdVecType& cellIdVec,
                                                           LastCell& lastCell) const
  {
    auto writeHit = WriteCellIdOnly<CellIdVecType>{ cellIdVec };
    return this->IterateLeavesImpl(coords, point, mode, cellIdVec, lastCell, writeHit);
  }

  template <typename CoordsPortalType>
  VISKORES_EXEC viskores::ErrorCode PointInCell(const CoordsPortalType& coords,
                                                const viskores::Vec3f& point,
                                                const viskores::Id& cid,
                                                viskores::Vec3f& pCoords) const
  {
    auto indices = this->CellSet.GetIndices(cid);
    auto pts = viskores::make_VecFromPortalPermute(&indices, coords);
    viskores::Vec3f pc;
    bool inside;
    auto status = viskores::exec::internal::CellLocatorPointInsideCell(
      point, this->CellSet.GetCellShape(cid), pts, pc, inside);
    if (status == viskores::ErrorCode::Success && inside)
    {
      pCoords = pc;
      return viskores::ErrorCode::Success;
    }

    return viskores::ErrorCode::CellNotFound;
  }

  template <typename CoordsPortalType>
  VISKORES_EXEC viskores::ErrorCode PointInLeaf(const CoordsPortalType& coords,
                                                const FloatVec3& point,
                                                const viskores::Id& leafIdx,
                                                viskores::Id& cellId,
                                                FloatVec3& pCoords) const
  {
    viskores::Id start = this->CellStartIndex.Get(leafIdx);
    viskores::Id end = start + this->CellCount.Get(leafIdx);

    for (viskores::Id i = start; i < end; ++i)
    {
      viskores::Vec3f pc;

      viskores::Id cid = this->CellIds.Get(i);
      if (this->PointInCell(coords, point, cid, pc) == viskores::ErrorCode::Success)
      {
        cellId = cid;
        pCoords = pc;
        return viskores::ErrorCode::Success;
      }
    }

    return viskores::ErrorCode::CellNotFound;
  }

  viskores::internal::cl_uniform_bins::Grid TopLevel;

  ReadPortal<DimVec3> LeafDimensions;
  ReadPortal<viskores::Id> LeafStartIndex;

  ReadPortal<viskores::Id> CellStartIndex;
  ReadPortal<viskores::Id> CellCount;
  ReadPortal<viskores::Id> CellIds;

  CellStructureType CellSet;
  viskores::cont::internal::CellLocatorCoordinatePortalVariant Coords;
};
}
} // viskores::exec

#endif //viskores_exec_CellLocatorTwoLevel_h
